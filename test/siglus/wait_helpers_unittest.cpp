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
#include "vm/future.hpp"
#include "vm/gc.hpp"
#include "vm/promise.hpp"

#include <memory>
#include <queue>

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
