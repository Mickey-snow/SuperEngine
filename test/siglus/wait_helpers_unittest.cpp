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

#include <gtest/gtest.h>

#include "libsiglus/bindings/wait_helpers.hpp"
#include "systems/event_backend.hpp"
#include "systems/event_system.hpp"
#include "vm/exception.hpp"
#include "vm/future.hpp"
#include "vm/gc.hpp"
#include "vm/promise.hpp"
#include "vm/vm.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

using namespace libsiglus::binding;

class QueueEventBackend : public IEventBackend {
 public:
  std::shared_ptr<Event> PollEvent() override {
    if (events_.empty())
      return std::make_shared<Event>(std::monostate{});

    std::shared_ptr<Event> event = events_.front();
    events_.pop();
    return event;
  }

  template <typename T>
  void Push(T event) {
    events_.emplace(std::make_shared<Event>(event));
  }

 private:
  std::queue<std::shared_ptr<Event>> events_;
};

class ImmediateTask : public CoroutineTask {
 public:
  ImmediateTask(serilang::VM& vm, int result, int* starts)
      : CoroutineTask(vm), result_(result), starts_(starts) {}

  TaskCoroutine Run() override {
    ++*starts_;
    co_return result_;
  }

 private:
  int result_;
  int* starts_;
};

class ScheduledTask : public CoroutineTask {
 public:
  ScheduledTask(serilang::VM& vm, int result, int* starts)
      : CoroutineTask(vm), result_(result), starts_(starts) {}

  TaskCoroutine Run() override {
    ++*starts_;
    co_await WaitFor(std::chrono::milliseconds(1));
    co_return result_;
  }

 private:
  int result_;
  int* starts_;
};

class StdExceptionThrowingTask : public CoroutineTask {
 public:
  explicit StdExceptionThrowingTask(serilang::VM& vm) : CoroutineTask(vm) {}

  TaskCoroutine Run() override {
    throw std::runtime_error("scheduled task failed");
    co_return 0;
  }
};

class RuntimeErrorThrowingTask : public CoroutineTask {
 public:
  explicit RuntimeErrorThrowingTask(serilang::VM& vm) : CoroutineTask(vm) {}

  TaskCoroutine Run() override {
    throw serilang::RuntimeError("scheduled task failed");
    co_return 0;
  }
};

TEST(WaitHandlerTest, SkipKeyWithoutCallbackIsNoop) {
  auto backend = std::make_unique<QueueEventBackend>();
  QueueEventBackend* backend_ptr = backend.get();
  EventSystem event_system(std::move(backend));
  auto gc = std::make_shared<serilang::GarbageCollector>();

  WaitHandler handler(gc, &event_system);
  backend_ptr->Push(KeyDown{KeyCode::RETURN});

  EXPECT_NO_THROW(event_system.ExecuteEventSystem());
}

TEST(WaitHandlerTest, FuturesShareTheHandlerPromise) {
  auto gc = std::make_shared<serilang::GarbageCollector>();
  WaitHandler handler(gc);

  serilang::Future* first = handler.GetFuture();
  serilang::Future* second = handler.GetFuture();

  EXPECT_EQ(first->promise, second->promise);

  handler.Resolve(serilang::Value(1));

  ASSERT_TRUE(first->promise->HasResult());
  ASSERT_TRUE(second->promise->HasResult());
  EXPECT_EQ(first->promise->result->value(), serilang::Value(1));
  EXPECT_EQ(second->promise->result->value(), serilang::Value(1));
}

TEST(WaitHandlerTest, PollingFutureResolvesImmediatelyWhenDone) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());

  serilang::Value value = MakePollingWaitFuture(vm, [] { return true; });
  auto* future = value.Get_if<serilang::Future>();

  ASSERT_NE(future, nullptr);
  ASSERT_TRUE(future->promise->HasResult());
  EXPECT_EQ(future->promise->result->value(), serilang::Value(0));
}

TEST(WaitHandlerTest, PollingFutureResolvesAfterPredicateChanges) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());
  bool done = false;
  serilang::Value result;

  serilang::Value value = MakePollingWaitFuture(vm, [&done] { return done; });
  serilang::Value awaiter;
  vm.Await(awaiter, value, [&result](const auto& outcome) {
    ASSERT_TRUE(outcome.has_value());
    result = outcome.value();
  });
  vm.scheduler_.PushCallbackAfter([&done] { done = true; },
                                  std::chrono::milliseconds(1));

  vm.Run();

  EXPECT_EQ(result, serilang::Value(0));
}

TEST(WaitHandlerTest, PollingFutureKeySkipResolvesWithOne) {
  auto expect_skip = [](auto event) {
    auto backend = std::make_unique<QueueEventBackend>();
    QueueEventBackend* backend_ptr = backend.get();
    EventSystem event_system(std::move(backend));
    serilang::VM vm(std::make_shared<serilang::GarbageCollector>());

    serilang::Value value =
        MakePollingWaitFuture(vm, [] { return false; }, true, &event_system);
    backend_ptr->Push(event);
    event_system.ExecuteEventSystem();

    auto* future = value.Get_if<serilang::Future>();
    ASSERT_NE(future, nullptr);
    ASSERT_TRUE(future->promise->HasResult());
    EXPECT_EQ(future->promise->result->value(), serilang::Value(1));
  };

  expect_skip(KeyDown{KeyCode::RETURN});
  expect_skip(KeyDown{KeyCode::SPACE});
  expect_skip(MouseDown{MouseButton::LEFT});
}

TEST(WaitHandlerTest, PollingFutureWithoutKeySkipIgnoresInput) {
  auto backend = std::make_unique<QueueEventBackend>();
  QueueEventBackend* backend_ptr = backend.get();
  EventSystem event_system(std::move(backend));
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());

  serilang::Value value =
      MakePollingWaitFuture(vm, [] { return false; }, false, &event_system);
  backend_ptr->Push(KeyDown{KeyCode::RETURN});
  event_system.ExecuteEventSystem();

  auto* future = value.Get_if<serilang::Future>();
  ASSERT_NE(future, nullptr);
  EXPECT_FALSE(future->promise->HasResult());
}

TEST(FutureBackedCoroutineTaskTest, StartsAfterMoveAndResolvesReturnedValue) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());
  std::vector<FutureBackedCoroutineTask> pending;
  int starts = 0;

  FutureBackedCoroutineTask task(
      std::make_unique<ImmediateTask>(vm, /*result=*/7, &starts));
  serilang::Future* future = task.MakeFuture(*vm.gc_);
  pending.emplace_back(std::move(task));

  serilang::Value awaiter;
  serilang::Value awaited(future);
  serilang::Value result;
  vm.Await(awaiter, awaited, [&result](const auto& outcome) {
    ASSERT_TRUE(outcome.has_value());
    result = outcome.value();
  });

  EXPECT_EQ(starts, 1);
  ASSERT_TRUE(future->promise->HasResult());
  EXPECT_EQ(result, serilang::Value(7));
  EXPECT_TRUE(pending.front().Done());
}

TEST(PendingCoroutineTasksTest, MakesFutureAndPrunesCompletedTasks) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());
  PendingCoroutineTasks pending;
  int starts = 0;

  serilang::Future* first = pending.MakeFuture(
      *vm.gc_, std::make_unique<ImmediateTask>(vm, 3, &starts));
  EXPECT_EQ(pending.size(), 1);

  serilang::Value awaiter;
  serilang::Value awaited(first);
  serilang::Value result;
  vm.Await(awaiter, awaited, [&result](const auto& outcome) {
    ASSERT_TRUE(outcome.has_value());
    result = outcome.value();
  });

  EXPECT_EQ(result, serilang::Value(3));
  EXPECT_EQ(starts, 1);

  serilang::Future* second = pending.MakeFuture(
      *vm.gc_, std::make_unique<ImmediateTask>(vm, 4, &starts));
  EXPECT_NE(second, nullptr);
  EXPECT_EQ(pending.size(), 1);
}

TEST(FutureBackedCoroutineTaskTest, ResolvesWhenScheduledRoutineCompletes) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());
  int starts = 0;

  FutureBackedCoroutineTask task(
      std::make_unique<ScheduledTask>(vm, /*result=*/9, &starts));
  serilang::Future* future = task.MakeFuture(*vm.gc_);
  serilang::Value awaiter;
  serilang::Value awaited(future);
  serilang::Value result;

  vm.Await(awaiter, awaited, [&result](const auto& outcome) {
    ASSERT_TRUE(outcome.has_value());
    result = outcome.value();
  });

  EXPECT_EQ(starts, 1);
  EXPECT_FALSE(future->promise->HasResult());

  vm.Run();

  ASSERT_TRUE(future->promise->HasResult());
  EXPECT_EQ(result, serilang::Value(9));
  EXPECT_TRUE(task.Done());
}

TEST(FutureBackedCoroutineTaskTest, RejectsRuntimeErrorWithThrownMessage) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());

  FutureBackedCoroutineTask task(
      std::make_unique<RuntimeErrorThrowingTask>(vm));
  serilang::Future* future = task.MakeFuture(*vm.gc_);
  serilang::Value awaiter;
  serilang::Value awaited(future);

  vm.Await(awaiter, awaited, [](const auto& outcome) {
    ASSERT_FALSE(outcome.has_value());
    EXPECT_EQ(outcome.error(), "scheduled task failed");
  });

  ASSERT_TRUE(future->promise->HasResult());
  ASSERT_FALSE(future->promise->result->has_value());
  EXPECT_EQ(future->promise->result->error(), "scheduled task failed");
  EXPECT_TRUE(task.Done());
}

TEST(FutureBackedCoroutineTaskTest, PropagatesNonRuntimeError) {
  serilang::VM vm(std::make_shared<serilang::GarbageCollector>());

  FutureBackedCoroutineTask task(
      std::make_unique<StdExceptionThrowingTask>(vm));
  serilang::Value awaiter;
  serilang::Value awaited(task.MakeFuture(*vm.gc_));

  EXPECT_THROW(vm.Await(awaiter, awaited, [](const auto&) {}),
               std::runtime_error);
}
