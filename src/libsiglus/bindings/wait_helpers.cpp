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
#include "vm/exception.hpp"
#include "vm/future.hpp"
#include "vm/gc.hpp"
#include "vm/promise.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sr = serilang;

namespace {

bool IsAdvanceInput(const Event& event) {
  return std::visit(overload(
                        [](const KeyDown& event) {
                          return event.code == KeyCode::RETURN ||
                                 event.code == KeyCode::SPACE;
                        },
                        [](const MouseDown& event) {
                          return event.button == MouseButton::LEFT;
                        },
                        [](const auto&) { return false; }),
                    event);
}

}  // namespace

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
        if (IsAdvanceInput(*event))
          Notify();
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

// ------------------------------------------------------------------------------

struct CoroutineTask::DelayAwaiter::State : public EventListener {
  State(std::coroutine_handle<> h) : h_(h) {}
  void Notify(WaitOutcome res) {
    if (result.has_value())
      return;
    result = res;
    h_.resume();  // will this result in recursion and create very deep
                  // callstack?
  }
  void OnEvent(std::shared_ptr<Event> event) override {
    if (IsAdvanceInput(*event))
      Notify(WaitOutcome::InterruptedByInput);
  }
  std::coroutine_handle<> h_;
  std::optional<WaitOutcome> result;
};

CoroutineTask::DelayAwaiter::DelayAwaiter(serilang::VM& vm,
                                          EventSystem* event_system,
                                          std::chrono::milliseconds delay,
                                          bool interrupt_on_input)
    : vm_(vm),
      event_system_(event_system),
      delay_(delay),
      interrupt_on_input_(interrupt_on_input) {}

void CoroutineTask::DelayAwaiter::await_suspend(std::coroutine_handle<> h) {
  state_ = std::make_shared<State>(h);
  std::weak_ptr<State> weak = state_;
  vm_.scheduler_.PushCallbackAfter(
      [weak] {
        if (auto state = weak.lock())
          state->Notify(WaitOutcome::Timeout);
      },
      delay_);
  if (event_system_ && interrupt_on_input_)
    event_system_->AddListener(weak);
}

CoroutineTask::WaitOutcome CoroutineTask::DelayAwaiter::await_resume() {
  return *state_->result;
}

CoroutineTask::CoroutineTask(serilang::VM& vm, EventSystem* event_system)
    : vm_(vm), event_system_(event_system) {}

CoroutineTask::DelayAwaiter CoroutineTask::WaitFor(
    std::chrono::milliseconds delay,
    bool interrupt_on_input) {
  return DelayAwaiter(vm_, event_system_, delay, interrupt_on_input);
}

void CoroutineTask::TaskCoroutine::promise_type::return_value(int value) {
  if (auto promise = completion_promise.lock())
    promise->Resolve(sr::Value(value));
}

void CoroutineTask::TaskCoroutine::promise_type::unhandled_exception() {
  try {
    throw;
  } catch (const sr::RuntimeError& e) {
    if (auto promise = completion_promise.lock()) {
      promise->Reject(e.message());
    } else {
      throw;
    }
  }
}

void CoroutineTask::TaskCoroutine::SetCompletionPromise(
    std::weak_ptr<serilang::Promise> promise) {
  if (h)
    h.promise().completion_promise = std::move(promise);
}

struct FutureBackedCoroutineTask::State {
  explicit State(std::unique_ptr<CoroutineTask> task)
      : task(std::move(task)), promise(std::make_shared<serilang::Promise>()) {}

  void Start() {
    if (started)
      return;
    started = true;
    coroutine = task->Run();
    coroutine.SetCompletionPromise(promise);
    coroutine.Start();
  }

  std::unique_ptr<CoroutineTask> task;
  std::shared_ptr<serilang::Promise> promise;
  CoroutineTask::TaskCoroutine coroutine;
  bool started = false;
};

FutureBackedCoroutineTask::FutureBackedCoroutineTask(
    std::unique_ptr<CoroutineTask> task)
    : state_(std::make_shared<State>(std::move(task))) {
  std::weak_ptr<State> weak_state = state_;
  state_->promise->initial_await = [weak_state](sr::VM& vm, sr::Value& awaiter,
                                                sr::Value& awaited) {
    // The coroutine starts only after the returned future is first awaited.
    if (auto state = weak_state.lock())
      state->Start();
    (void)vm, (void)awaiter, (void)awaited;
  };
}

void FutureBackedCoroutineTask::Start() {
  if (state_)
    state_->Start();
}

sr::Future* FutureBackedCoroutineTask::MakeFuture(sr::GarbageCollector& gc) {
  auto fut = gc.Allocate<sr::Future>();
  fut->promise = state_->promise;
  return fut;
}

bool FutureBackedCoroutineTask::Done() const {
  return !state_ || state_->promise->HasResult();
}

sr::Future* PendingCoroutineTasks::MakeFuture(
    sr::GarbageCollector& gc,
    FutureBackedCoroutineTask pending) {
  PruneDone();
  sr::Future* future = pending.MakeFuture(gc);
  tasks_.emplace_back(std::move(pending));
  return future;
}

sr::Future* PendingCoroutineTasks::MakeFuture(
    sr::GarbageCollector& gc,
    std::unique_ptr<CoroutineTask> task) {
  PruneDone();
  FutureBackedCoroutineTask pending(std::move(task));
  sr::Future* future = pending.MakeFuture(gc);
  tasks_.emplace_back(std::move(pending));
  return future;
}

void PendingCoroutineTasks::PruneDone() {
  std::erase_if(tasks_, [](const FutureBackedCoroutineTask& task) {
    return task.Done();
  });
}

std::size_t PendingCoroutineTasks::size() const { return tasks_.size(); }

}  // namespace libsiglus::binding
