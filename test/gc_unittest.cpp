// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "vm/call_frame.hpp"
#include "vm/chunk.hpp"
#include "vm/gc.hpp"
#include "vm/value_internal/closure.hpp"
#include "vm/value_internal/dict.hpp"
#include "vm/value_internal/fiber.hpp"
#include "vm/value_internal/list.hpp"
#include "vm/vm.hpp"

namespace serilang_test {
using namespace serilang;

// A tiny IObject subclass that counts how many instances are alive.
class DummyObject : public IObject {
 public:
  static int& aliveCount() {
    static int count = 0;
    return count;
  }

  DummyObject() { ++aliveCount(); }
  ~DummyObject() override { --aliveCount(); }

  static constexpr inline ObjType objtype = ObjType::Dummy;
  constexpr ObjType Type() const noexcept override { return objtype; }
};

class GCTest : public ::testing::Test {
 protected:
  VM vm{nullptr};
  GarbageCollector& gc = vm.gc_;

  template <typename T, typename... Args>
  T* Alloc(Args... args) {
    auto* ptr = gc.Allocate<T>(std::forward<Args>(args)...);
    return ptr;
  }
};

TEST_F(GCTest, AllocatedBytes) {
  size_t before = vm.gc_.AllocatedBytes();
  Alloc<DummyObject>();
  Alloc<DummyObject>();

  EXPECT_EQ(vm.gc_.AllocatedBytes(), before + 2 * sizeof(DummyObject));
  vm.CollectGarbage();
  EXPECT_EQ(vm.gc_.AllocatedBytes(), before);
}

TEST_F(GCTest, Sweep) {
  DummyObject::aliveCount() = 0;
  Alloc<DummyObject>();
  EXPECT_EQ(DummyObject::aliveCount(), 1);

  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}

TEST_F(GCTest, MarkValue) {
  DummyObject::aliveCount() = 0;
  auto* d = Alloc<DummyObject>();
  EXPECT_EQ(DummyObject::aliveCount(), 1);

  // Mark by wrapping in Value
  vm.last_ = Value(d);
  vm.CollectGarbage();
  // Still alive because it was marked
  EXPECT_EQ(DummyObject::aliveCount(), 1);

  vm.last_ = Value();
  vm.CollectGarbage();
  // Next sweep without marking should collect it
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}

TEST_F(GCTest, MarkList) {
  DummyObject::aliveCount() = 0;
  auto* d1 = Alloc<DummyObject>();
  auto* d2 = Alloc<DummyObject>();
  EXPECT_EQ(DummyObject::aliveCount(), 2);

  // Build a List containing both
  std::vector<Value> items = {Value(d1), Value(d2)};
  auto* listObj = Alloc<List>(std::move(items));

  vm.last_ = Value(listObj);
  vm.CollectGarbage();
  // Both DummyObjects survive because listVal was marked, which recursed into
  // items
  EXPECT_EQ(DummyObject::aliveCount(), 2);

  // If we sweep again without re-marking the list, they all die:
  vm.last_ = Value(1);
  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}

TEST_F(GCTest, MarkDict) {
  DummyObject::aliveCount() = 0;
  auto* d1 = Alloc<DummyObject>();
  auto* d2 = Alloc<DummyObject>();
  EXPECT_EQ(DummyObject::aliveCount(), 2);

  // Build a Dict containing both
  std::unordered_map<std::string, Value> mp;
  mp["one"] = Value(d1);
  mp["two"] = Value(d2);
  auto* dictObj = Alloc<Dict>(std::move(mp));
  Value dictVal(dictObj);

  vm.last_ = Value(dictObj);
  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 2);

  vm.last_ = Value();
  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}

TEST_F(GCTest, MarkGlobalsRoot) {
  DummyObject::aliveCount() = 0;
  auto* d = Alloc<DummyObject>();
  EXPECT_EQ(DummyObject::aliveCount(), 1);

  // Place into VM globals
  vm.globals_["foo"] = Value(d);
  vm.CollectGarbage();
  // Still alive
  EXPECT_EQ(DummyObject::aliveCount(), 1);

  // Remove global and recollect
  vm.globals_.clear();
  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}

TEST_F(GCTest, MarkFibresAndClosures) {
  DummyObject::aliveCount() = 0;
  // Create a one-shot closure and fiber
  auto chunk = std::make_shared<Chunk>();
  auto* cl = Alloc<Closure>(chunk);
  auto* f = Alloc<Fiber>();
  f->frames.emplace_back(CallFrame(cl));

  auto* d1 = Alloc<DummyObject>();
  auto* d2 = Alloc<DummyObject>();
  auto* d3 = Alloc<DummyObject>();
  f->stack.emplace_back(d1);
  f->last = Value(d2);
  cl->chunk->const_pool.emplace_back(d3);

  // Register fiber in VM
  vm.fibres_.clear();
  vm.fibres_.push_back(f);

  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 3);

  // Next collection with no fiber should collect both
  vm.fibres_.clear();
  vm.CollectGarbage();
  EXPECT_EQ(DummyObject::aliveCount(), 0);
}
}  // namespace serilang_test
