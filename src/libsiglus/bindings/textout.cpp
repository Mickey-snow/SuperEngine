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

#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "srbind/module.hpp"
#include "systems/system.hpp"
#include "systems/text_page.hpp"
#include "systems/text_system.hpp"
#include "vm/future.hpp"
#include "vm/vm.hpp"

#include <utf8.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

namespace {
class SiglusTextoutState
    : public std::enable_shared_from_this<SiglusTextoutState> {
 public:
  SiglusTextoutState(sr::VM& vm,
                     System* system,
                     std::shared_ptr<Gameexe> local_config,
                     std::string text)
      : vm_(vm),
        system_(system),
        local_config_(std::move(local_config)),
        text_(std::move(text)),
        pos_(text_.cbegin()) {}

  sr::Value Start() {
    if (!system_ || text_.empty())
      return MakeResolvedFuture(*vm_.gc_);

    wait_handler_ =
        std::make_shared<WaitHandler>(vm_.gc_, system_->event_ptr().get());
    std::weak_ptr<SiglusTextoutState> weak = shared_from_this();
    wait_handler_->OnKey([weak] {
      if (auto state = weak.lock())
        state->flush_requested_ = true;
    });

    sr::Value future(wait_handler_->GetFuture());
    if (ShouldFlushText()) {
      Complete();
      return future;
    }

    Schedule();
    return future;
  }

 private:
  void Schedule() {
    int speed = system_ ? system_->text().message_speed() : 0;
    if (local_config_) {
      if (auto val = (*local_config_)("message_speed").Int())
        speed = val.value();
    }

    vm_.scheduler_.PushCallbackAfter(
        [self = shared_from_this()] { self->Poll(); },
        std::chrono::milliseconds(std::max(speed, 1)));
  }

  void Poll() {
    if (finished_)
      return;

    try {
      if (ShouldFlushText() || flush_requested_) {
        Complete();
        return;
      }

      if (DisplayNext())
        Resolve(0);
      else
        Schedule();
    } catch (const std::exception& e) {
      Reject(e.what());
    } catch (...) {
      Reject("Siglus textout failed with an unknown exception");
    }
  }

  bool DisplayNext() {
    if (pos_ == text_.cend())
      return true;

    auto cur = pos_;
    auto next = cur;
    utf8::next(next, text_.cend());
    std::string current(cur, next);
    std::string rest(next, text_.cend());

    TextPage& page = system_->text().GetCurrentPage();
    page.Character(current, rest);
    pos_ = next;
    return pos_ == text_.cend();
  }

  void Complete() {
    while (!finished_ && pos_ != text_.cend())
      DisplayNext();
    Resolve(0);
  }

  void Resolve(int result) {
    if (finished_)
      return;
    finished_ = true;
    wait_handler_->Resolve(sr::Value(result));
  }

  void Reject(std::string message) {
    if (finished_)
      return;
    finished_ = true;
    wait_handler_->Reject(std::move(message));
  }

  bool ShouldFlushText() const {
    if (!system_)
      return true;
    TextSystem& text = system_->text();
    if (system_->ShouldFastForward())
      return true;
    if (text.message_no_wait() || text.script_message_nowait())
      return true;
    if (local_config_) {
      if ((*local_config_)("message_nowait").Int().value_or(0))
        return true;
    }
    return false;
  }

  sr::VM& vm_;
  System* system_;
  std::shared_ptr<Gameexe> local_config_;
  std::string text_;
  std::string::const_iterator pos_;
  std::shared_ptr<WaitHandler> wait_handler_;
  bool flush_requested_ = false;
  bool finished_ = false;
};
}  // namespace

void BindTextout(SiglusRuntime& runtime) {
  sb::module_ m(runtime.vm->gc_.get(), runtime.vm->globals_.get());
  m.def("__builtin_textout",
        [system = runtime.system.get(), local_config = runtime.local_config](
            sr::VM& vm, int kidoku, std::string text) -> sr::Value {
          (void)kidoku;  // TODO: Add kidoku support
          auto state = std::make_shared<SiglusTextoutState>(
              vm, system, local_config, std::move(text));
          return state->Start();
        });
}

RLVM_REGISTER(SiglusBindingRegistry, "textout", BindTextout)

}  // namespace libsiglus::binding
