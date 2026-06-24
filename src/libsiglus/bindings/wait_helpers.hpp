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
#include <vector>

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

// Base class for VM-backed asynchronous work implemented as a C++ coroutine.
// Wrap it in FutureBackedCoroutineTask when the coroutine should be exposed as
// a serilang::Future and started on first await.
class CoroutineTask {
 public:
  CoroutineTask(serilang::VM& vm, EventSystem* event_system = nullptr);
  virtual ~CoroutineTask() = default;

  enum class WaitOutcome { Timeout, InterruptedByInput };

  struct DelayAwaiter {
    DelayAwaiter(serilang::VM& vm,
                 EventSystem* event_system,
                 std::chrono::milliseconds delay,
                 bool interrupt_on_input);
    inline bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h);
    WaitOutcome await_resume();

    struct State;
    std::shared_ptr<State> state_;
    serilang::VM& vm_;
    EventSystem* event_system_;
    std::chrono::milliseconds delay_;
    bool interrupt_on_input_;
  };
  DelayAwaiter WaitFor(std::chrono::milliseconds delay,
                       bool interrupt_on_input = false);

  struct TaskCoroutine {
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type {
      inline TaskCoroutine get_return_object() {
        return TaskCoroutine{handle::from_promise(*this)};
      }
      inline std::suspend_always initial_suspend() noexcept { return {}; }
      inline std::suspend_always final_suspend() noexcept { return {}; }
      void return_value(int value);
      void unhandled_exception();
      std::weak_ptr<serilang::Promise> completion_promise;
    };

    TaskCoroutine() = default;
    explicit TaskCoroutine(handle h) : h(h) {}
    TaskCoroutine(const TaskCoroutine&) = delete;
    TaskCoroutine& operator=(const TaskCoroutine&) = delete;
    TaskCoroutine(TaskCoroutine&& other) noexcept
        : h(std::exchange(other.h, nullptr)) {}
    TaskCoroutine& operator=(TaskCoroutine&& other) noexcept {
      if (this == &other)
        return *this;
      if (h)
        h.destroy();
      h = std::exchange(other.h, nullptr);
      return *this;
    }
    ~TaskCoroutine() {
      if (h)
        h.destroy();
    }
    inline operator bool() const { return h != nullptr; }
    inline void Start() { h.resume(); }
    void SetCompletionPromise(std::weak_ptr<serilang::Promise> promise);
    handle h = nullptr;
  };

  virtual TaskCoroutine Run() = 0;

 private:
  serilang::VM& vm_;
  EventSystem* event_system_;
};

class FutureBackedCoroutineTask {
 public:
  FutureBackedCoroutineTask(std::unique_ptr<CoroutineTask> task);
  FutureBackedCoroutineTask(const FutureBackedCoroutineTask&) = delete;
  FutureBackedCoroutineTask& operator=(const FutureBackedCoroutineTask&) =
      delete;
  FutureBackedCoroutineTask(FutureBackedCoroutineTask&&) noexcept = default;
  FutureBackedCoroutineTask& operator=(FutureBackedCoroutineTask&&) noexcept =
      default;

  serilang::Future* MakeFuture(serilang::GarbageCollector& gc);
  bool Done() const;

 private:
  struct State;
  std::shared_ptr<State> state_;
};

class PendingCoroutineTasks {
 public:
  serilang::Future* MakeFuture(serilang::GarbageCollector& gc,
                               std::unique_ptr<CoroutineTask> task);
  void PruneDone();
  std::size_t size() const;

 private:
  std::vector<FutureBackedCoroutineTask> tasks_;
};

};  // namespace libsiglus::binding
