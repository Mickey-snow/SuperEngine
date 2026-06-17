// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include "libsiglus/bindings/wait_helpers.hpp"

#include "systems/event_system.hpp"
#include "utilities/overload.hpp"
#include "vm/future.hpp"
#include "vm/gc.hpp"
#include "vm/promise.hpp"
#include "vm/vm.hpp"

#include <utility>

namespace libsiglus::binding {
namespace sr = serilang;

WaitHandler::WaitHandler(std::shared_ptr<sr::GarbageCollector> gc,
                         EventSystem* event_system)
    : gc_(std::move(gc)), promise_(std::make_shared<sr::Promise>()), cb_() {
  if (event_system) {
    struct SkipKeyListener : public EventListener {
      WaitHandler& wh_;
      SkipKeyListener(WaitHandler& wh) : wh_(wh) {}
      void Notify() {
        std::function<void()> cb = wh_.cb_;
        if (cb)
          cb();
      }
      void OnEvent(std::shared_ptr<Event> event) override {
        std::visit(overload(
                       [this](const KeyDown& event) {
                         if (event.code == KeyCode::RETURN ||
                             event.code == KeyCode::SPACE) {
                           Notify();
                         }
                       },
                       [this](const MouseDown& event) {
                         if (event.button == MouseButton::LEFT)
                           Notify();
                       },
                       [](const auto&) {}),
                   *event);
      }
    };
    listener_ = std::make_shared<SkipKeyListener>(*this);
    event_system->AddListener(listener_);
  }
}

WaitHandler::~WaitHandler() { Resolve(0); }

sr::Future* WaitHandler::GetFuture() const {
  sr::Future* future = gc_->Allocate<sr::Future>();
  future->promise = promise_;
  return future;
}

void WaitHandler::Resolve(sr::Value result) {
  if (promise_)
    promise_->Resolve(std::move(result));
}

void WaitHandler::Reject(std::string error) {
  if (promise_)
    promise_->Reject(std::move(error));
}

// ------------------------------------------------------------------------------

sr::Value MakeResolvedFuture(sr::GarbageCollector& gc, int result) {
  sr::Future* future = gc.Allocate<sr::Future>();
  future->promise->Resolve(sr::Value(result));
  return sr::Value(future);
}

sr::Value MakePollingWaitFuture(sr::VM& vm,
                                std::function<bool()> done,
                                bool key_skip,
                                EventSystem* event_system,
                                std::chrono::milliseconds poll_interval) {
  if (!done || done())
    return MakeResolvedFuture(*vm.gc_);

  struct PollState : public std::enable_shared_from_this<PollState> {
    sr::VM& vm;
    std::function<bool()> done;
    std::chrono::milliseconds poll_interval;
    std::shared_ptr<WaitHandler> wait_handler;
    bool finished = false;

    PollState(sr::VM& vm,
              std::function<bool()> done,
              std::chrono::milliseconds poll_interval)
        : vm(vm),
          done(std::move(done)),
          poll_interval(poll_interval.count() < 0 ? std::chrono::milliseconds(0)
                                                  : poll_interval) {}

    void Resolve(int result) {
      if (finished)
        return;

      finished = true;
      wait_handler->Resolve(sr::Value(result));
    }

    void Reject(std::string error) {
      if (finished)
        return;

      finished = true;
      wait_handler->Reject(std::move(error));
    }

    void Poll() {
      if (finished)
        return;

      try {
        if (done()) {
          Resolve(0);
          return;
        }
      } catch (const std::exception& e) {
        Reject(e.what());
        return;
      } catch (...) {
        Reject("wait predicate threw an unknown exception");
        return;
      }

      vm.scheduler_.PushCallbackAfter(
          [self = shared_from_this()] { self->Poll(); }, poll_interval);
    }
  };

  auto state = std::make_shared<PollState>(vm, std::move(done), poll_interval);
  state->wait_handler =
      std::make_shared<WaitHandler>(vm.gc_, key_skip ? event_system : nullptr);
  if (key_skip) {
    std::weak_ptr<PollState> weak_state = state;
    state->wait_handler->OnKey([weak_state] {
      if (auto state = weak_state.lock())
        state->Resolve(1);
    });
  }

  sr::Value future(state->wait_handler->GetFuture());
  vm.scheduler_.PushCallbackAfter([state] { state->Poll(); },
                                  state->poll_interval);
  return future;
}

}  // namespace libsiglus::binding
