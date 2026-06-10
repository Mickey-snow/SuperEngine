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

#include <utility>

namespace libsiglus::binding {
namespace sr = serilang;

sr::Value MakeResolvedFuture(sr::GarbageCollector& gc, int result) {
  sr::Future* future = gc.Allocate<sr::Future>();
  future->promise->Resolve(sr::Value(result));
  return sr::Value(future);
}

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

}  // namespace libsiglus::binding
