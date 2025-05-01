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

#include "vm/instruction.hpp"
#include "vm/vm.hpp"

namespace serilang_test {

using namespace serilang;

// Small helper: wrapper to create Value array
template <typename... Ts>
  requires(std::constructible_from<Value, Ts> && ...)
inline static std::vector<Value> value_vector(Ts&&... param) {
  return std::vector<Value>{Value(std::forward<Ts>(param))...};
}

// Small helper: run a chunk, return the final value that the *main*
/// fiber reported via `last`.
static Value run_and_get(const std::shared_ptr<Chunk>& chunk) {
  VM vm(chunk);
  vm.Run();
  return vm.main_fiber->last;
}

TEST(VMTest, BinaryAdd) {
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(1.0, 2.0);
  chunk->code = {Push{0}, Push{1}, BinaryOp{Op::Add}, Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 3.0);
}

TEST(VMTest, UnaryNeg) {
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(5.0);
  chunk->code = {Push{0}, UnaryOp{Op::Sub}, Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, -5.0);
}

TEST(VMTest, DupAndSwap) {
  // Program:   (1 2) swap ⇒ (2 1) dup ⇒ (2 1 1) add,add ⇒ 2+1+1 = 4
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(1.0, 2.0);
  chunk->code = {Push{0},  // 1
                 Push{1},  // 2
                 Swap{},   // 2 1
                 Dup{},    // 2 1 1
                 BinaryOp{Op::Add},
                 BinaryOp{Op::Add},
                 Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 4.0);
}

TEST(VMTest, StoreLoadLocal) {
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(42.0);

  // bootstrap main‑closure: we need one local slot
  //   local0 = 42;  return local0;
  chunk->code = {Push{0}, StoreLocal{0}, LoadLocal{0}, Return{}};

  // Trick: tell the VM that “main” closure has 1 local
  VM vm(chunk);
  vm.main_fiber->stack.resize(1);
  vm.main_fiber->frames[0].closure->nlocals = 1;
  vm.Run();

  Value out = vm.main_fiber->last;
  EXPECT_EQ(out, 42.0);
}

TEST(VMTest, FunctionCall) {
  // Layout:
  //   0: MakeClosure(entry=5)   ; push fn
  //   3: Call0
  //   4: Return
  //
  //   5: Push 7
  //   7: Return
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(7.0);
  chunk->code = {MakeClosure{3, 0, 0, 0}, Call{0}, Return{}, Push{0}, Return{}};

  VM vm(chunk);
  vm.Run();
  EXPECT_EQ(vm.main_fiber->last, 7.0);
}

TEST(VMTest, TailCall) {
  //   inner(): return 99
  //   outer(): return inner()   (via TailCall)
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(99.0);

  // indices
  constexpr uint32_t kInner = 5;
  constexpr uint32_t kOuter = 3;

  chunk->code = {// main → call outer()
                 MakeClosure{kOuter, 0, 0, 0}, Call{0}, Return{},

                 // outer()
                 MakeClosure{kInner, 0, 0, 0}, TailCall{0},

                 // inner()
                 Push{0}, Return{}};

  VM vm(chunk);
  vm.Run();
  EXPECT_EQ(vm.main_fiber->last, 99.0);
}

//      if (1 < 2) push 222 else push 111
TEST(VMTest, ConditionalJump) {
  auto chunk = std::make_shared<Chunk>();
  chunk->const_pool = value_vector(1.0, 2.0, 111.0, 222.0);

  //  0 1 < 2 ?
  //  if‑true  +2   (skip the ‘else’ push)
  //  push 222
  //  jump +1 (skip then‑push)
  //  push 111
  //  return
  chunk->code = {Push{0},        Push{1}, BinaryOp{Op::Less},
                 JumpIfFalse{2}, Push{3}, Jump{1},
                 Push{2},        Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 222.0);
}

TEST(VMTest, ReturnNil) {
  auto chunk = std::make_shared<Chunk>();

  // return
  chunk->code = {Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, std::monostate());
}

TEST(VMTest, CallNative) {
  auto chunk = std::make_shared<Chunk>();

  int call_count = 0;
  auto fn = std::make_shared<NativeFunction>(
      "my_function", [&](VM& vm, std::vector<Value> args) {
	++call_count;
        EXPECT_EQ(args.size(), 2);
        EXPECT_EQ(args[0], 1);
        EXPECT_EQ(args[1], "foo");
        return Value();
      });

  chunk->const_pool = value_vector(fn, 1, "foo");
  chunk->code = {Push{0}, Push{1}, Push{2}, Call{2}, Return{}};

  Value out = run_and_get(chunk);
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(out, std::monostate());
}

}  // namespace serilang_test
