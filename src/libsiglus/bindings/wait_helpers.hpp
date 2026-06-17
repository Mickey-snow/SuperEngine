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

#pragma once

#include "core/event_listener.hpp"
#include "vm/value.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

class EventSystem;

namespace serilang {
class GarbageCollector;
struct Future;
struct Promise;
class VM;
};  // namespace serilang

namespace libsiglus::binding {

class WaitHandler {
 public:
  WaitHandler(std::shared_ptr<serilang::GarbageCollector> gc,
              EventSystem* event_system = nullptr);
  ~WaitHandler();

  inline void OnKey(std::function<void()> callback = {}) { cb_.swap(callback); }
  serilang::Future* GetFuture() const;
  void Resolve(serilang::Value result);
  void Reject(std::string error);

 private:
  std::shared_ptr<serilang::GarbageCollector> gc_;
  std::shared_ptr<serilang::Promise> promise_;
  std::function<void()> cb_;
  std::shared_ptr<EventListener> listener_;
};

serilang::Value MakeResolvedFuture(serilang::GarbageCollector& gc,
                                   int result = 0);
serilang::Value MakePollingWaitFuture(
    serilang::VM& vm,
    std::function<bool()> done,
    bool key_skip = false,
    EventSystem* event_system = nullptr,
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds(5));

};  // namespace libsiglus::binding
