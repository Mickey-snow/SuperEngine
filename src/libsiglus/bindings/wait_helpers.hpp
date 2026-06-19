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
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

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

// ------------------------------------------------------------------------------

class ITask {
 public:
  ITask(serilang::VM& vm, EventSystem* es = nullptr);
  virtual ~ITask() = default;

  enum class WaitResult { Timeout, Key };

  struct Awaitable {
    Awaitable(serilang::VM& vm,
              EventSystem* es,
              std::chrono::milliseconds ms,
              bool can_skip_by_key);
    // TODO: By design, this awaitable object should be one-shot, not reusable
    // how do we enforce this on language level?
    inline bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h);
    WaitResult await_resume();

    struct State;
    std::shared_ptr<State> state_;
    serilang::VM& vm_;
    EventSystem* es_;
    std::chrono::milliseconds ms_;
    bool can_skip_by_key_;
  };
  Awaitable Schedule(std::chrono::milliseconds ms,
                     bool can_skip_by_key = false);

  struct Routine {
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type {
      inline Routine get_return_object() {
        return Routine{handle::from_promise(*this)};
      }
      inline std::suspend_always initial_suspend() noexcept { return {}; }
      inline std::suspend_always final_suspend() noexcept { return {}; }
      void return_value(int value);
      void unhandled_exception();
      std::weak_ptr<serilang::Promise> completion_promise;
    };

    Routine() = default;
    explicit Routine(handle h) : h(h) {}
    Routine(const Routine&) = delete;
    Routine& operator=(const Routine&) = delete;
    Routine(Routine&& other) noexcept : h(std::exchange(other.h, nullptr)) {}
    Routine& operator=(Routine&& other) noexcept {
      if (this == &other)
        return *this;
      if (h)
        h.destroy();
      h = std::exchange(other.h, nullptr);
      return *this;
    }
    ~Routine() {
      if (h)
        h.destroy();
    }
    inline operator bool() const { return h != nullptr; }
    inline void start() { h.resume(); }
    void SetCompletionPromise(std::weak_ptr<serilang::Promise> promise);
    handle h = nullptr;
  };

  virtual Routine GetRoutine() = 0;

 private:
  serilang::VM& vm_;
  EventSystem* es_;
};

class PackagedTask {
 public:
  PackagedTask(std::unique_ptr<ITask> task);
  PackagedTask(const PackagedTask&) = delete;
  PackagedTask& operator=(const PackagedTask&) = delete;
  PackagedTask(PackagedTask&&) noexcept = default;
  PackagedTask& operator=(PackagedTask&&) noexcept = default;

  serilang::Future* MakeFuture(serilang::GarbageCollector& gc);
  bool Done() const;

 private:
  struct State;
  std::shared_ptr<State> state_;
};

};  // namespace libsiglus::binding
