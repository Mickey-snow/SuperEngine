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
#include "libsiglus/bindings/wait_helpers.hpp"
#include "srbind/srbind.hpp"
#include "systems/event_system.hpp"
#include "systems/system.hpp"
#include "systems/text_page.hpp"
#include "systems/text_system.hpp"
#include "systems/text_window.hpp"
#include "vm/future.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

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

bool AutoModeEnabled(System* system, const std::shared_ptr<Gameexe>& cfg) {
  return system &&
         (system->text().auto_mode() != 0 || ConfigFlag(cfg, "auto_mode"));
}

void ClearActiveMessageWindow(System* system) {
  if (!system)
    return;

  TextSystem& text = system->text();
  const int active_window = text.active_window();
  text.Snapshot();
  text.GetTextWindow(active_window)->ClearWin();
  text.NewPageOnWindow(active_window);
}

class MwndWaitState : public std::enable_shared_from_this<MwndWaitState> {
 public:
  MwndWaitState(sr::VM& vm,
                System* system,
                std::shared_ptr<Gameexe> local_config,
                bool clear_after)
      : vm_(vm),
        system_(system),
        local_config_(std::move(local_config)),
        clear_after_(clear_after) {}

  sr::Value Start() {
    if (!system_) {
      if (clear_after_)
        ClearActiveMessageWindow(system_);
      return MakeResolvedFuture(*vm_.gc_);
    }

    if (MessageNowait(system_, local_config_)) {
      if (clear_after_)
        ClearActiveMessageWindow(system_);
      return MakeResolvedFuture(*vm_.gc_);
    }

    TextSystem& text = system_->text();
    text.set_in_pause_state(true);
    start_ticks_ = system_->event().GetTicks();
    auto_time_ =
        text.GetAutoTime(text.GetCurrentPage().number_of_chars_on_page());

    wait_handler_ =
        std::make_shared<WaitHandler>(vm_.gc_, system_->event_ptr().get());
    std::weak_ptr<MwndWaitState> weak = shared_from_this();
    wait_handler_->OnKey([weak] {
      if (auto state = weak.lock())
        state->Complete(1);
    });

    sr::Value future(wait_handler_->GetFuture());
    Schedule();
    return future;
  }

  ~MwndWaitState() {
    if (!finished_ && system_)
      system_->text().set_in_pause_state(false);
  }

 private:
  void Schedule() {
    vm_.scheduler_.PushCallbackAfter(
        [self = shared_from_this()] { self->Poll(); },
        std::chrono::milliseconds(5));
  }

  void Poll() {
    if (finished_)
      return;

    try {
      if (MessageNowait(system_, local_config_)) {
        Complete(0);
        return;
      }

      if (AutoModeEnabled(system_, local_config_)) {
        const unsigned int elapsed = system_->event().GetTicks() - start_ticks_;
        if (elapsed >= static_cast<unsigned int>(std::max(auto_time_, 0))) {
          Complete(0);
          return;
        }
      }

      Schedule();
    } catch (const std::exception& e) {
      Reject(e.what());
    } catch (...) {
      Reject("Siglus mwnd wait failed with an unknown exception");
    }
  }

  void Complete(int result) {
    if (finished_)
      return;

    finished_ = true;
    if (system_)
      system_->text().set_in_pause_state(false);
    if (clear_after_)
      ClearActiveMessageWindow(system_);
    wait_handler_->Resolve(sr::Value(result));
  }

  void Reject(std::string message) {
    if (finished_)
      return;

    finished_ = true;
    if (system_)
      system_->text().set_in_pause_state(false);
    wait_handler_->Reject(std::move(message));
  }

  sr::VM& vm_;
  System* system_;
  std::shared_ptr<Gameexe> local_config_;
  bool clear_after_;
  unsigned int start_ticks_ = 0;
  int auto_time_ = 0;
  std::shared_ptr<WaitHandler> wait_handler_;
  bool finished_ = false;
};

sr::Value MakeMwndWaitFuture(sr::VM& vm,
                             System* system,
                             std::shared_ptr<Gameexe> local_config,
                             bool clear_after) {
  return std::make_shared<MwndWaitState>(vm, system, std::move(local_config),
                                         clear_after)
      ->Start();
}

}  // namespace

class SiglusMwnd {
  std::shared_ptr<TextWindow> text_win_;
  TextPage page;

 public:
  SiglusMwnd(std::shared_ptr<TextWindow> wd, System& sys)
      : text_win_(wd), page(sys.gameexe(), wd) {}

  bool DisplayCharacter(std::string current, std::string rest) {
    return page.Character(current, rest);
  }
  void DisplayRuby(std::string text) {
    text_win_->MarkRubyBegin();
    text_win_->DisplayRubyText(text);
  }

  void Clear() { text_win_->ClearWin(); }

  void SetFontColor(int r, int g, int b) {
    RGBColour rgb(r, g, b);
    text_win_->SetFontColor(rgb);
  }
};

void BindMwnd(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm, "mwnd");
  auto system = runtime.system.get();
  auto local_config = runtime.local_config;
  m.def("close", [] { throw std::runtime_error("TODO"); });
  m.def("close_nowait", [] { throw std::runtime_error("TODO"); });
  m.def("close_wait", [] { throw std::runtime_error("TODO"); });
  m.def("msg_block",
        [] { throw std::runtime_error("TODO: Siglus message blocking"); });
  m.def("msg_pp_block",
        [] { throw std::runtime_error("TODO: Siglus page-break blocking"); });
  m.def("msg_wait",
        [](sr::VM& vm) -> sr::Value { return MakeResolvedFuture(*vm.gc_); });
  m.def("pp", [system, local_config](sr::VM& vm) -> sr::Value {
    return MakeMwndWaitFuture(vm, system, local_config, false);
  });
  m.def("r", [system, local_config](sr::VM& vm) -> sr::Value {
    if (ConfigFlag(local_config, "ignore_r"))
      return MakeResolvedFuture(*vm.gc_);
    return MakeMwndWaitFuture(vm, system, local_config, true);
  });
  m.def("page", [system, local_config](sr::VM& vm) -> sr::Value {
    return MakeMwndWaitFuture(vm, system, local_config, true);
  });
  m.def("clear", [system] { ClearActiveMessageWindow(system); });
}

RLVM_REGISTER(SiglusBindingRegistry, "0_mwnd", BindMwnd)

}  // namespace libsiglus::binding
