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

#include "vm/disassembler.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

namespace serilang_test {

using namespace serilang;

class VMTest : public ::testing::Test {
 protected:
  VM vm = VM::Create();
  GarbageCollector& gc = vm.gc_;

  // Small helper: wrapper to create Value array
  template <typename... Ts>
    requires(std::constructible_from<Value, Ts> && ...)
  inline std::vector<Value> value_vector(Ts&&... param) {
    return std::vector<Value>{Value(std::forward<Ts>(param))...};
  }

  inline void append_ins(Code* chunk, std::vector<Instruction> ins) {
    for (const auto& it : ins)
      chunk->Append(it);
  }

  Value run_and_get(Code* chunk) { return vm.Evaluate(chunk); }
};

TEST_F(VMTest, BinaryAdd) {
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(1.0, 2.0);
  append_ins(chunk, {Push{0}, Push{1}, BinaryOp{Op::Add}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 3.0);
}

TEST_F(VMTest, UnaryNeg) {
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(5.0);
  append_ins(chunk, {Push{0}, UnaryOp{Op::Sub}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, -5.0);
}

TEST_F(VMTest, DupAndSwap) {
  // Program:   (1 2) swap ⇒ (2 1) dup ⇒ (2 1 1) add,add ⇒ 2+1+1 = 4
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(1.0, 2.0);
  append_ins(chunk, {Push{0},  // 1
                     Push{1},  // 2
                     Swap{},   // 2 1
                     Dup{},    // 2 1 1
                     BinaryOp{Op::Add}, BinaryOp{Op::Add}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 4.0);
}

TEST_F(VMTest, StoreLoadLocal) {
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(42.0);

  // bootstrap main‑closure: we need one local slot
  //   local0 = 42;  return local0;
  append_ins(chunk, {Push{0}, StoreLocal{0}, LoadLocal{0}, Return{}});

  // Trick: tell the VM that “main” closure has 1 local
  VM vm = VM::Create();
  vm.AddFiber(chunk);
  vm.main_fiber_->stack.resize(1);
  vm.Run();

  Value out = vm.main_fiber_->last;
  EXPECT_EQ(out, 42.0);
}

TEST_F(VMTest, FunctionCall) {
  // Layout:
  //   0 : MakeClosure(entry=13)   ; push fn
  //   9 : Call0
  //  12 : Return
  //
  //  13 : Push 7
  //  18 : Return
  GarbageCollector gc;
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(7.0, Value(chunk));
  append_ins(chunk, {Push(1), MakeFunction{.entry = 13}, Call{0, 0}, Return{},
                     Push{0}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 7.0);
}

//      if (1 < 2) push 222 else push 111
TEST_F(VMTest, ConditionalJump) {
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(1.0, 2.0, 111.0, 222.0);

  //  0 1 < 2 ?
  //  if‑true  +2   (skip the ‘else’ push)
  //  push 222
  //  jump +1 (skip then‑push)
  //  push 111
  //  return
  append_ins(chunk, {Push{0}, Push{1}, BinaryOp{Op::Less}, JumpIfFalse{10},
                     Push{3}, Jump{5}, Push{2}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, 222.0);
}

TEST_F(VMTest, ReturnNil) {
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(std::monostate(), "2.unused");

  // return nil;
  append_ins(chunk, {Push{0}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(out, std::monostate());
}

TEST_F(VMTest, CallNative) {
  auto* chunk = gc.Allocate<Code>();

  int call_count = 0;
  auto fn = gc.Allocate<NativeFunction>(
      "my_function", [&](Fiber& f, std::vector<Value> args,
                         std::unordered_map<std::string, Value> kwargs) {
        ++call_count;
        EXPECT_EQ(args.size(), 2);
        EXPECT_EQ(args[0], 1);
        EXPECT_EQ(args[1], "foo");
        return Value();
      });

  chunk->const_pool = value_vector(fn, 1, "foo");
  append_ins(chunk, {Push{0}, Push{1}, Push{2}, Call{2, 0}, Return{}});

  Value out = run_and_get(chunk);
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(out, std::monostate());
}

}  // namespace serilang_test
