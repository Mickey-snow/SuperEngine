// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "libsiglus/bindings/registry.hpp"

#include "core/colour.hpp"
#include "core/gameexe.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "srbind/srbind.hpp"
#include "systems/event_system.hpp"
#include "systems/sound_system.hpp"
#include "systems/system.hpp"
#include "systems/text_page.hpp"
#include "systems/text_system.hpp"
#include "systems/text_window.hpp"
#include "vm/dict.hpp"
#include "vm/future.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

namespace {

bool ConfigFlag(const std::shared_ptr<Gameexe>& cfg, std::string const& key) {
  if (!cfg)
    return false;
  return (*cfg)(key).Int().value_or(0) != 0;
}

bool MessageNowait(System* system, const std::shared_ptr<Gameexe>& cfg) {
  if (!system)
    return true;
  TextSystem& text = system->text();
  return system->ShouldFastForward() || text.message_no_wait() ||
         text.script_message_nowait() || ConfigFlag(cfg, "message_nowait");
}

int ConfigInt(const std::shared_ptr<Gameexe>& cfg,
              std::string const& key,
              int fallback) {
  if (!cfg)
    return fallback;
  return (*cfg)(key).Int().value_or(fallback);
}

bool AutoModeEnabled(System* system, const std::shared_ptr<Gameexe>& cfg) {
  return system &&
         (system->text().auto_mode() != 0 || ConfigFlag(cfg, "auto_mode"));
}

int CurrentPageCharCount(System* system) {
  if (!system)
    return 0;
  return system->text().GetCurrentPage().number_of_chars_on_page();
}

struct MwndMessageState {
  bool block_started = false;
  bool clear_ready = false;
  int auto_mode_base_chars = 0;
  int current_koe = -1;
  int current_character = -1;
  bool current_koe_played = false;
  bool current_koe_no_auto_mode = false;
};

void ClearKoeState(const std::shared_ptr<MwndMessageState>& state) {
  if (!state)
    return;

  state->current_koe = -1;
  state->current_character = -1;
  state->current_koe_played = false;
  state->current_koe_no_auto_mode = false;
}

void MarkMessageClearReady(System* system,
                           const std::shared_ptr<MwndMessageState>& state) {
  if (!state)
    return;

  state->clear_ready = true;
  state->block_started = false;
  state->auto_mode_base_chars = CurrentPageCharCount(system);
  ClearKoeState(state);
}

void MarkMessageNovelClear(System* system,
                           const std::shared_ptr<MwndMessageState>& state) {
  if (!state)
    return;

  state->block_started = false;
  state->auto_mode_base_chars = CurrentPageCharCount(system);
  ClearKoeState(state);
}

struct KoeCallParams {
  int koe = 0;
  int character = -1;
  bool no_auto_mode = false;
  static KoeCallParams ParseFrom(std::vector<sr::Value> raw_args) {
    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
    KoeCallParams params;

    if (!packet.args.empty())
      params.koe = AsInt(packet.args[0]).value_or(0);
    if (packet.args.size() > 1)
      params.character = AsInt(packet.args[1]).value_or(-1);

    ForEachKeywordId(packet.kwargs, [&](int id, const sr::Value& value) {
      switch (id) {
        case 0:
          params.no_auto_mode = AsInt(value).value_or(0) != 0;
          break;
        default:
          break;
      }
    });

    return params;
  }
};

class MwndWaitTask : public CoroutineTask {
 public:
  MwndWaitTask(sr::VM& vm,
               System* system,
               std::shared_ptr<Gameexe> local_config,
               std::shared_ptr<MwndMessageState> message_state,
               bool mark_clear_ready_after)
      : CoroutineTask(vm, system ? system->event_ptr().get() : nullptr),
        system_(system),
        local_config_(std::move(local_config)),
        message_state_(std::move(message_state)),
        mark_clear_ready_after_(mark_clear_ready_after) {}

  TaskCoroutine Run() override {
    if (!system_ || MessageNowait(system_, local_config_)) {
      MarkClearReadyAfterWait();
      co_return 0;
    }

    PauseGuard pause(system_);

    try {
      TextSystem& text = system_->text();
      const unsigned int start_ticks = system_->event().GetTicks();
      const int auto_time = text.GetAutoTime(AutoModeCharCount());

      while (true) {
        if ((co_await WaitFor(std::chrono::milliseconds(5), true)) ==
            WaitOutcome::InterruptedByInput) {
          pause.Reset();
          MarkClearReadyAfterWait();
          co_return 1;
        }

        if (MessageNowait(system_, local_config_)) {
          pause.Reset();
          MarkClearReadyAfterWait();
          co_return 0;
        }

        if (AutoModeEnabled(system_, local_config_)) {
          const unsigned int elapsed =
              system_->event().GetTicks() - start_ticks;
          if (elapsed >= static_cast<unsigned int>(std::max(auto_time, 0))) {
            pause.Reset();
            MarkClearReadyAfterWait();
            co_return 0;
          }
        }
      }
    } catch (...) {
      pause.Reset();
      throw;
    }
  }

 private:
  class PauseGuard {
   public:
    explicit PauseGuard(System* system)
        : text_(system ? &system->text() : nullptr) {
      if (text_)
        text_->set_in_pause_state(true);
    }

    ~PauseGuard() { Reset(); }

    void Reset() {
      if (!text_)
        return;

      text_->set_in_pause_state(false);
      text_ = nullptr;
    }

   private:
    TextSystem* text_;
  };

  void MarkClearReadyAfterWait() {
    if (mark_clear_ready_after_)
      MarkMessageClearReady(system_, message_state_);
  }

  int AutoModeCharCount() {
    const int configured_count =
        ConfigInt(local_config_, "auto_mode_moji_cnt", 0);
    if (configured_count > 0)
      return configured_count;

    const int current_count = CurrentPageCharCount(system_);
    if (!message_state_)
      return current_count;
    return std::max(current_count - message_state_->auto_mode_base_chars, 0);
  }

  System* system_;
  std::shared_ptr<Gameexe> local_config_;
  std::shared_ptr<MwndMessageState> message_state_;
  bool mark_clear_ready_after_;
};

struct MwndBindingState {
  MwndBindingState(sr::VM& vm,
                   System* system,
                   std::shared_ptr<Gameexe> local_config)
      : vm(vm),
        system(system),
        local_config(std::move(local_config)),
        message_state(std::make_shared<MwndMessageState>()) {}

  sr::Value Wait(bool mark_clear_ready_after) {
    auto wait_task = std::make_unique<MwndWaitTask>(
        vm, system, local_config, message_state, mark_clear_ready_after);
    return sr::Value(pending_waits.MakeFuture(*vm.gc_, std::move(wait_task)));
  }

  void PlayKoe(std::vector<sr::Value> raw_args) {
    auto params = KoeCallParams::ParseFrom(std::move(raw_args));
    const bool character_enabled =
        !system || params.character < 0 ||
        system->sound().ShouldUseKoeForCharacter(params.character) != 0;

    if (message_state) {
      message_state->current_koe = params.koe;
      message_state->current_character = params.character;
      message_state->current_koe_played = character_enabled;
      message_state->current_koe_no_auto_mode = params.no_auto_mode;
    }

    if (!system)
      return;

    if (params.character >= 0)
      system->sound().KoePlay(params.koe, params.character);
    else
      system->sound().KoePlay(params.koe);
    system->text().GetCurrentPage().KoeMarker(params.koe);
  }

  sr::Value WaitKoe(sr::VM& vm, bool key_skip) {
    auto done = [system = system] {
      return !system || !system->sound().KoePlaying();
    };
    return MakePollingWaitFuture(vm, std::move(done), key_skip,
                                 system ? system->event_ptr().get() : nullptr);
  }

  sr::VM& vm;
  System* system;
  std::shared_ptr<Gameexe> local_config;
  std::shared_ptr<MwndMessageState> message_state;
  PendingCoroutineTasks pending_waits;
};

}  // namespace

void BindMwnd(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm, "mwnd");
  auto state = std::make_shared<MwndBindingState>(vm, runtime.system.get(),
                                                  runtime.local_config);
  auto close = [state] {
    auto* sys = state->system;
    if (!sys)
      return;
    TextSystem& text = sys->text();
    text.set_in_pause_state(false);
    text.HideTextWindow(text.active_window());
  };
  m.def("close", close);
  m.def("close_nowait", close);
  m.def("close_wait", close);
  m.def("end_close", [] {});
  m.def("msg_block", [state] {
    auto msgstate = state->message_state;
    if (!msgstate)
      return;

    if (msgstate->block_started)
      return;

    if (msgstate->clear_ready) {
      auto* system = state->system;
      if (!system)
        return;

      TextSystem& text = system->text();
      const int active_window = text.active_window();
      text.Snapshot();
      text.GetTextWindow(active_window)->ClearWin();
      text.NewPageOnWindow(active_window);

      msgstate->clear_ready = false;
    }

    // Legacy Siglus also updates savepoints/backlog/read flags here. Those
    // subsystems do not exist in the current Siglus runtime yet.
    msgstate->block_started = true;
    msgstate->auto_mode_base_chars = CurrentPageCharCount(state->system);
  });
  m.def("msg_pp_block", [state] {
    auto msgstate = state->message_state;
    if (!msgstate)
      return;
    msgstate->auto_mode_base_chars = CurrentPageCharCount(state->system);
  });
  m.def("msg_wait",
        [](sr::VM& vm) -> sr::Value { return MakeResolvedFuture(*vm.gc_); });
  m.def("pp", [state](sr::VM&) -> sr::Value { return state->Wait(false); });
  m.def("r", [state](sr::VM& vm) -> sr::Value {
    if (ConfigFlag(state->local_config, "ignore_r"))
      return MakeResolvedFuture(*vm.gc_);
    return state->Wait(false);
  });
  m.def("page", [state](sr::VM&) -> sr::Value { return state->Wait(true); });
  m.def("clear", [state] {
    MarkMessageClearReady(state->system, state->message_state);
  });
  m.def("novel_clear", [state] {
    MarkMessageNovelClear(state->system, state->message_state);
  });
  m.def(
      "koe",
      [state](std::vector<sr::Value> args) { state->PlayKoe(std::move(args)); },
      sb::vararg);
  m.def(
      "koe_play_wait",
      [state](sr::VM& vm, std::vector<sr::Value> args) -> sr::Value {
        state->PlayKoe(std::move(args));
        return state->WaitKoe(vm, false);
      },
      sb::vararg);
  m.def(
      "koe_play_wait_key",
      [state](sr::VM& vm, std::vector<sr::Value> args) -> sr::Value {
        state->PlayKoe(std::move(args));
        return state->WaitKoe(vm, true);
      },
      sb::vararg);
  m.def("set_waku", [](int waku){
    // TODO
  });
}

RLVM_REGISTER(SiglusBindingRegistry, "0_mwnd", BindMwnd)

}  // namespace libsiglus::binding
