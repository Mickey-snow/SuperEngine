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
#include "libsiglus/bindings/common.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/siglus_scene_renderer.hpp"
#include "srbind/srbind.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/sdl/sdl_surface.hpp"
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
#include <cmath>
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

struct CallPacket {
  std::vector<sr::Value> args;
  const sr::Dict* kwargs = nullptr;
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

std::optional<int> ParseKeywordId(const sr::Value& key) {
  const sr::String* str = key.Get_if<sr::String>();
  if (!str)
    return std::nullopt;

  std::string_view text = str->str_;
  if (!text.empty() && text.front() == '_')
    text.remove_prefix(1);
  if (text.empty())
    return std::nullopt;

  int result = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, result);
  if (ec != std::errc() || ptr != end)
    return std::nullopt;
  return result;
}

CallPacket DecodePacket(std::vector<sr::Value> raw) {
  if (raw.size() == 3 && raw[1].Get_if<sr::List>() &&
      raw[2].Get_if<sr::Dict>()) {
    const sr::List* args = raw[1].Get_if<sr::List>();
    return CallPacket{.args = args->items, .kwargs = raw[2].Get_if<sr::Dict>()};
  }

  return CallPacket{.args = std::move(raw)};
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

    CallPacket packet = DecodePacket(std::move(raw_args));
    ApplyPositional(packet.args, params, masked);
    ApplyKeywords(packet.kwargs, params);
    last_ = std::move(params);

    if (!stage_)
      return MakeResolvedFuture(vm, 0);

    if (!system_) {
      stage_->Wipe(last_.begin_order, last_.end_order, last_.begin_layer,
                   last_.end_layer);
      EndCurrent(0);
      return MakeResolvedFuture(vm, 0);
    }

    GraphicsSystem& graphics = system_->graphics();
    before_surface_ = graphics.RenderToSurface();
    stage_->Wipe(last_.begin_order, last_.end_order, last_.begin_layer,
                 last_.end_layer);
    after_surface_ = graphics.RenderToSurface();

    if (ShouldCompleteImmediately()) {
      EndCurrent(0);
      return MakeResolvedFuture(vm, 0);
    }

    active_ = true;
    start_ticks_ = system_->event().GetTicks();
    progress_ = ComputeProgress(last_.start_time);

    if (!last_.wait_flag)
      return MakeResolvedFuture(vm, 0);
    return MakePendingFuture(vm, last_.key_wait_mode);
  }

  sr::Value Wait(sr::VM& vm, std::vector<sr::Value> raw_args) {
    if (!active_)
      return MakeResolvedFuture(vm, 0);

    int key_wait_mode = -1;
    CallPacket packet = DecodePacket(std::move(raw_args));
    if (!packet.args.empty())
      key_wait_mode = AsInt(packet.args.front()).value_or(-1);
    if (packet.kwargs) {
      for (const auto& [key, value] : packet.kwargs->map) {
        const std::optional<int> id = ParseKeywordId(key);
        if (id && *id == 0)
          key_wait_mode = AsInt(value).value_or(-1);
      }
    }

    return MakePendingFuture(vm, key_wait_mode);
  }

  void EndCurrent(int result) {
    active_ = false;
    progress_ = 1.0;
    before_surface_.reset();
    after_surface_.reset();
    DisableKeySkip();

    if (stage_)
      stage_->next_objects.Clear();

    ResolveWaiters(result);
  }

  int Check() const { return active_ ? 1 : 0; }

  bool UpdateAndRender() {
    if (!active_ || !system_)
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
    RenderCrossfade(system_->graphics());
    return true;
  }

 private:
  struct KeySkipListener : public EventListener {
    explicit KeySkipListener(Impl* owner) : owner_(owner) {}

    void OnEvent(std::shared_ptr<Event> event) override {
      if (!owner_ || !owner_->active_ || !event)
        return;

      std::visit(overload(
                     [this](const KeyDown& event) {
                       if (event.code == KeyCode::RETURN ||
                           event.code == KeyCode::SPACE) {
                         owner_->EndCurrent(1);
                       }
                     },
                     [this](const MouseDown& event) {
                       if (event.button == MouseButton::LEFT)
                         owner_->EndCurrent(1);
                     },
                     [](const auto&) {}),
                 *event);
    }

    Impl* owner_;
  };

  static sr::Value MakeResolvedFuture(sr::VM& vm, int result) {
    sr::Future* future = vm.gc_->Allocate<sr::Future>();
    future->promise->Resolve(sr::Value(result));
    return sr::Value(future);
  }

  sr::Value MakePendingFuture(sr::VM& vm, int key_wait_mode) {
    sr::Future* future = vm.gc_->Allocate<sr::Future>();
    waiters_.emplace_back(future->promise);
    if (KeySkipEnabled(key_wait_mode))
      EnableKeySkip();
    return sr::Value(future);
  }

  void ResolveWaiters(int result) {
    for (auto& waiter : waiters_)
      waiter->Resolve(sr::Value(result));
    waiters_.clear();
  }

  bool KeySkipEnabled(int key_wait_mode) const {
    if (key_wait_mode == 0)
      return false;
    if (key_wait_mode == 1)
      return true;
    return system_ && system_->graphics().should_skip_animations() != 0;
  }

  void EnableKeySkip() {
    if (!system_ || key_listener_)
      return;

    key_listener_ = std::make_shared<KeySkipListener>(this);
    system_->event().AddListener(key_listener_);
  }

  void DisableKeySkip() {
    if (!system_ || !key_listener_)
      return;

    system_->event().RemoveListener(key_listener_);
    key_listener_.reset();
  }

  bool ShouldCompleteImmediately() const {
    return !before_surface_ || !after_surface_ || last_.wipe_time <= 0 ||
           last_.start_time >= last_.wipe_time ||
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

  void RenderCrossfade(GraphicsSystem& graphics) const {
    if (!before_surface_ || !after_surface_)
      return;

    const Rect rect = graphics.screen_rect();
    before_surface_->RenderToScreen(rect, rect, 255);
    const int alpha =
        std::clamp(static_cast<int>(std::lround(progress_ * 255.0)), 0, 255);
    after_surface_->RenderToScreen(rect, rect, alpha);
  }

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
    if (!kwargs)
      return;

    for (const auto& [key, value] : kwargs->map) {
      const std::optional<int> id = ParseKeywordId(key);
      if (!id)
        continue;

      switch (*id) {
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
    }
  }

  bool active_ = false;
  WipeParams last_;
  unsigned int start_ticks_ = 0;
  double progress_ = 1.0;
  System* system_ = nullptr;
  Stage* stage_ = nullptr;
  std::shared_ptr<SDLSurface> before_surface_;
  std::shared_ptr<SDLSurface> after_surface_;
  std::vector<std::shared_ptr<sr::Promise>> waiters_;
  std::shared_ptr<EventListener> key_listener_;
};

SiglusWipe::SiglusWipe(System* system, Stage* stage)
    : impl_(std::make_unique<Impl>(system, stage)) {}
SiglusWipe::~SiglusWipe() = default;

sr::Value SiglusWipe::wipe(sr::VM& vm, std::vector<sr::Value> args) {
  return impl_->Start(vm, std::move(args), false, false);
}
sr::Value SiglusWipe::wipe_all(sr::VM& vm, std::vector<sr::Value> args) {
  return impl_->Start(vm, std::move(args), false, true);
}
sr::Value SiglusWipe::wipe_mask(sr::VM& vm, std::vector<sr::Value> args) {
  return impl_->Start(vm, std::move(args), true, false);
}
sr::Value SiglusWipe::wipe_mask_all(sr::VM& vm,
                                    std::vector<sr::Value> args) {
  return impl_->Start(vm, std::move(args), true, true);
}
void SiglusWipe::end(std::vector<sr::Value>) { impl_->EndCurrent(0); }
sr::Value SiglusWipe::wait(sr::VM& vm, std::vector<sr::Value> args) {
  return impl_->Wait(vm, std::move(args));
}
int SiglusWipe::check(std::vector<sr::Value>) const { return impl_->Check(); }
bool SiglusWipe::UpdateAndRender() { return impl_->UpdateAndRender(); }

void BindWipe(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());

  runtime.wipe =
      std::make_unique<SiglusWipe>(runtime.system.get(), runtime.stage.get());
  if (runtime.renderer)
    runtime.renderer->SetWipe(runtime.wipe.get());

  auto wipe = m.bind_instance("wipe", runtime.wipe.get());
  wipe.def("wipe", &SiglusWipe::wipe, sb::vararg);
  wipe.def("wipe_all", &SiglusWipe::wipe_all, sb::vararg);
  wipe.def("wipe_mask", &SiglusWipe::wipe_mask, sb::vararg);
  wipe.def("wipe_mask_all", &SiglusWipe::wipe_mask_all, sb::vararg);
  wipe.def("end", &SiglusWipe::end, sb::vararg);
  wipe.def("wait", &SiglusWipe::wait, sb::vararg);
  wipe.def("check", &SiglusWipe::check, sb::vararg);
}

RLVM_REGISTER(SiglusBindingRegistry, "wipe", BindWipe)

}  // namespace libsiglus::binding
