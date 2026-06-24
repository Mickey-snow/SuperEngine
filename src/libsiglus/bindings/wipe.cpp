// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// -----------------------------------------------------------------------

#include "libsiglus/bindings/wipe.hpp"

#include "core/event.hpp"
#include "core/event_listener.hpp"
#include "core/stage.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "libsiglus/siglus_scene_renderer.hpp"
#include "srbind/srbind.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "utilities/overload.hpp"
#include "vm/dict.hpp"
#include "vm/future.hpp"
#include "vm/list.hpp"
#include "vm/promise.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

namespace {

constexpr int kWipeOptionMax = 8;

struct WipeParams {
  int wipe_type = 0;
  int wipe_time = 500;
  int speed_mode = 0;
  int start_time = 0;
  std::array<int, kWipeOptionMax> option{};
  int begin_order = 0;
  int end_order = 0;
  int begin_layer = std::numeric_limits<int>::min();
  int end_layer = std::numeric_limits<int>::max();
  bool wait_flag = true;
  int key_wait_mode = -1;
  int with_low_order = 0;
  std::string mask_file;
};

void CopyOptions(const sr::Value& value,
                 std::array<int, kWipeOptionMax>& options) {
  const sr::List* list = value.Get_if<sr::List>();
  if (!list) {
    options[0] = AsInt(value).value_or(0);
    return;
  }

  const std::size_t count =
      std::min(list->items.size(), static_cast<std::size_t>(kWipeOptionMax));
  for (std::size_t i = 0; i < count; ++i)
    options[i] = AsInt(list->items[i]).value_or(0);
}

}  // namespace

struct SiglusWipe::Impl {
  Impl(System* system, Stage* stage) : system_(system), stage_(stage) {}
  ~Impl() { EndCurrent(0); }

  sr::Value Start(sr::VM& vm,
                  std::vector<sr::Value> raw_args,
                  bool masked,
                  bool all) {
    EndCurrent(0);

    WipeParams params;
    if (all)
      params.end_order = std::numeric_limits<int>::max();

    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
    ApplyPositional(packet.args, params, masked);
    ApplyKeywords(packet.kwargs, params);
    last_ = std::move(params);

    if (!stage_)
      return MakeResolvedFuture(*vm.gc_);

    stage_->Wipe(last_.begin_order, last_.end_order, last_.begin_layer,
                 last_.end_layer);

    if (!system_) {
      ClearWipeState();
      return MakeResolvedFuture(*vm.gc_);
    }

    if (ShouldCompleteImmediately()) {
      ClearWipeState();
      return MakeResolvedFuture(*vm.gc_);
    }

    wh_ = std::make_unique<WaitHandler>(vm.gc_, system_->event_ptr().get());
    start_ticks_ = system_->event().GetTicks();
    progress_ = ComputeProgress(last_.start_time);

    if (!last_.wait_flag)
      return MakeResolvedFuture(*vm.gc_);

    SetKeySkip(last_.key_wait_mode);
    return sr::Value(wh_->GetFuture());
  }

  sr::Value Wait(sr::VM& vm, std::vector<sr::Value> raw_args) {
    if (!wh_)
      return MakeResolvedFuture(*vm.gc_);

    int key_wait_mode = -1;
    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
    if (!packet.args.empty())
      key_wait_mode = AsInt(packet.args.front()).value_or(-1);
    ForEachKeywordId(packet.kwargs, [&](int id, const sr::Value& value) {
      if (id == 0)
        key_wait_mode = AsInt(value).value_or(-1);
    });
    SetKeySkip(key_wait_mode);

    return sr::Value(wh_->GetFuture());
  }

  void EndCurrent(int result) {
    if (!wh_)
      return;

    wh_->Resolve(result);
    wh_ = nullptr;
    ClearWipeState();
  }

  void ClearWipeState() {
    progress_ = 1.0;

    if (stage_)
      stage_->next_objects.Clear();
  }

  bool Update() {
    if (!wh_ || !system_)
      return false;

    if (system_->ShouldFastForward() ||
        system_->graphics().should_skip_animations()) {
      EndCurrent(0);
      return false;
    }

    const int elapsed = ElapsedTime();
    if (elapsed >= last_.wipe_time) {
      EndCurrent(0);
      return false;
    }

    progress_ = ComputeProgress(elapsed);
    return true;
  }

  void SetKeySkip(int key_wait_mode) {
    if (!wh_)
      return;

    bool enabled;
    if (key_wait_mode == 0)
      enabled = false;
    else if (key_wait_mode == 1)
      enabled = true;
    else
      enabled = system_ && system_->graphics().should_skip_animations() != 0;

    if (enabled)
      wh_->OnKey([this] { EndCurrent(1); });
    else
      wh_->OnKey();
  }

  bool ShouldCompleteImmediately() const {
    return last_.wipe_time <= 0 || last_.start_time >= last_.wipe_time ||
           system_->graphics().should_skip_animations() ||
           system_->ShouldFastForward();
  }

  int ElapsedTime() const {
    const unsigned int now = system_->event().GetTicks();
    return last_.start_time + static_cast<int>(now - start_ticks_);
  }

  double ComputeProgress(int elapsed) const {
    if (last_.wipe_time <= 0)
      return 1.0;

    const double t =
        std::clamp(static_cast<double>(elapsed) / last_.wipe_time, 0.0, 1.0);
    switch (last_.speed_mode) {
      case 0:
        return t;
      case 1:
        return t * t;
      case 2:
        return 1.0 - (1.0 - t) * (1.0 - t);
      default:
        return 0.0;
    }
  }

  inline bool IsActive() const { return wh_ != nullptr; }

  void ApplyPositional(const std::vector<sr::Value>& args,
                       WipeParams& params,
                       bool masked) {
    if (masked) {
      if (args.size() > 0)
        params.mask_file = AsString(args[0]);
      if (args.size() > 1)
        params.wipe_type = AsInt(args[1]).value_or(0);
      if (args.size() > 2)
        params.wipe_time = AsInt(args[2]).value_or(0);
      if (args.size() > 3)
        params.speed_mode = AsInt(args[3]).value_or(0);
      if (args.size() > 4)
        CopyOptions(args[4], params.option);
    } else {
      if (args.size() > 0)
        params.wipe_type = AsInt(args[0]).value_or(0);
      if (args.size() > 1)
        params.wipe_time = AsInt(args[1]).value_or(0);
      if (args.size() > 2)
        params.speed_mode = AsInt(args[2]).value_or(0);
      if (args.size() > 3)
        CopyOptions(args[3], params.option);
    }
  }

  void ApplyKeywords(const sr::Dict* kwargs, WipeParams& params) {
    ForEachKeywordId(kwargs, [&](int id, const sr::Value& value) {
      switch (id) {
        case 0:
          params.wipe_type = AsInt(value).value_or(0);
          break;
        case 1:
          params.wipe_time = AsInt(value).value_or(0);
          break;
        case 2:
          params.speed_mode = AsInt(value).value_or(0);
          break;
        case 3:
          CopyOptions(value, params.option);
          break;
        case 4:
          params.begin_order = AsInt(value).value_or(0);
          break;
        case 5:
          params.end_order = AsInt(value).value_or(0);
          break;
        case 6:
          params.begin_layer = AsInt(value).value_or(0);
          break;
        case 7:
          params.end_layer = AsInt(value).value_or(0);
          break;
        case 8:
          params.wait_flag = AsInt(value).value_or(0) != 0;
          break;
        case 9:
          params.key_wait_mode = AsInt(value).value_or(0);
          break;
        case 10:
          params.with_low_order = AsInt(value).value_or(0);
          break;
        case 11:
          params.start_time = AsInt(value).value_or(0);
          break;
        default:
          break;
      }
    });
  }

  WipeParams last_;
  unsigned int start_ticks_ = 0;
  double progress_ = 1.0;
  System* system_ = nullptr;
  Stage* stage_ = nullptr;
  std::unique_ptr<WaitHandler> wh_;
};

SiglusWipe::SiglusWipe(System* system, Stage* stage)
    : impl_(std::make_unique<Impl>(system, stage)) {}
SiglusWipe::~SiglusWipe() = default;
bool SiglusWipe::Update() { return impl_->Update(); }
bool SiglusWipe::IsActive() const { return impl_->IsActive(); }
double SiglusWipe::Progress() const { return impl_->progress_; }

void BindWipe(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());

  runtime.wipe =
      std::make_unique<SiglusWipe>(runtime.system.get(), runtime.stage.get());
  if (runtime.renderer)
    runtime.renderer->SetWipe(runtime.wipe.get());

  auto wipe = m.bind_instance("wipe", runtime.wipe.get());
  wipe.def(
      "wipe",
      [](SiglusWipe* wipe, sr::VM& vm, std::vector<sr::Value> args) {
        return wipe->impl_->Start(vm, std::move(args), false, false);
      },
      sb::vararg);
  wipe.def(
      "wipe_all",
      [](SiglusWipe* wipe, sr::VM& vm, std::vector<sr::Value> args) {
        return wipe->impl_->Start(vm, std::move(args), false, true);
      },
      sb::vararg);
  wipe.def(
      "wipe_mask",
      [](SiglusWipe* wipe, sr::VM& vm, std::vector<sr::Value> args) {
        return wipe->impl_->Start(vm, std::move(args), true, false);
      },
      sb::vararg);
  wipe.def(
      "wipe_mask_all",
      [](SiglusWipe* wipe, sr::VM& vm, std::vector<sr::Value> args) {
        return wipe->impl_->Start(vm, std::move(args), true, true);
      },
      sb::vararg);
  wipe.def(
      "end",
      [](SiglusWipe* wipe, std::vector<sr::Value> args) {
        wipe->impl_->EndCurrent(0);
      },
      sb::vararg);
  wipe.def(
      "wait",
      [](SiglusWipe* wipe, sr::VM& vm, std::vector<sr::Value> args) {
        return wipe->impl_->Wait(vm, std::move(args));
      },
      sb::vararg);
  wipe.def(
      "check",
      [](SiglusWipe* wipe, std::vector<sr::Value> args) {
        const int ret = wipe->IsActive() ? 1 : 0;
        return sr::Value(ret);
      },
      sb::vararg);
}

RLVM_REGISTER(SiglusBindingRegistry, "wipe", BindWipe)

}  // namespace libsiglus::binding
