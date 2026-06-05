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

#include "core/stage.hpp"
#include "libsiglus/bindings/common.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "srbind/srbind.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "vm/dict.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

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

class SiglusWipe {
 public:
  explicit SiglusWipe(System* system) : system_(system) {}

  void wipe(std::vector<sr::Value> args) {
    Start(std::move(args), false, false);
  }
  void wipe_all(std::vector<sr::Value> args) {
    Start(std::move(args), false, true);
  }
  void wipe_mask(std::vector<sr::Value> args) {
    Start(std::move(args), true, false);
  }
  void wipe_mask_all(std::vector<sr::Value> args) {
    Start(std::move(args), true, true);
  }

  void end(std::vector<sr::Value>) { EndCurrent(); }

  int wait(std::vector<sr::Value>) {
    EndCurrent();
    return 0;
  }

  int check(std::vector<sr::Value>) const { return active_ ? 1 : 0; }

 private:
  void Start(std::vector<sr::Value> raw_args, bool masked, bool all) {
    EndCurrent();

    WipeParams params;
    if (all)
      params.end_order = std::numeric_limits<int>::max();

    CallPacket packet = DecodePacket(std::move(raw_args));
    ApplyPositional(packet.args, params, masked);
    ApplyKeywords(packet.kwargs, params);

    last_ = std::move(params);
    active_ = true;

    if (system_) {
      system_->graphics().stage().Wipe(last_.begin_order, last_.end_order,
                                       last_.begin_layer, last_.end_layer);
    }

    // TODO(siglus): This state-only implementation fast-forwards wipes. It
    // intentionally ignores visual wipe animation, mask rendering, wipe
    // type/options, start time, speed mode, with_low_order, and key-skip wait
    // behavior until the renderer has Siglus transition support.
    EndCurrent();
  }

  void EndCurrent() {
    active_ = false;
    if (system_) {
      auto& stage = system_->graphics().stage();
      stage.next_objects.Clear();
    }
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
  System* system_ = nullptr;
};

void BindWipe(Context&, SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());

  auto wipe = m.bind_instance(
      "wipe", std::make_unique<SiglusWipe>(runtime.system.get()));
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
