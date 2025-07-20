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
  GarbageCollector& gc = *vm.gc_;

  // Small helper: wrapper to create Value array
  template <typename... Ts>
    requires(std::constructible_from<Value, Ts> && ...)
  inline std::vector<Value> value_vector(Ts&&... param) {
    return std::vector<Value>{Value(std::forward<Ts>(param))...};
  }

  inline void append_ins(Code* chunk, std::initializer_list<Instruction> ins) {
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
  Value out = vm.Evaluate(chunk);
  EXPECT_EQ(out, 42.0);
}

TEST_F(VMTest, FunctionCall) {
  // Layout:
  //  0  PUSH                 1  ; <code>
  //  5  MAKE_FUNCION          entry=13  nargs=0
  // 22  CALL                  nargs=0  nkwargs=0
  // 31  RETURN
  // 33  PUSH                 0  ; <double: 7.000000>
  // 38  RETURN
  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(7.0, Value(chunk));
  append_ins(chunk, {Push(1), MakeFunction{.entry = 33}, Call{0, 0}, Return{},
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

TEST_F(VMTest, MultipleFibres) {
  auto* chunk1 = gc.Allocate<Code>();
  chunk1->const_pool = value_vector(1);
  append_ins(chunk1, {Push{0}, Return{}});
  // fiber1: return 1;
  auto* chunk2 = gc.Allocate<Code>();
  chunk2->const_pool = value_vector(3, 2, 1);
  append_ins(chunk2, {Push{0}, Push{1}, Push{2}, BinaryOp(Op::Add),
                      BinaryOp(Op::Mul), Return{}});
  // fiber2: return 3*(2+1);

  Fiber* f1 = vm.AddFiber(chunk1);
  Fiber* f2 = vm.AddFiber(chunk2);
  f1->state = FiberState::Running;
  f2->state = FiberState::Running;
  std::ignore = vm.Run();

  EXPECT_EQ(f1->state, FiberState::Dead);
  EXPECT_EQ(f2->state, FiberState::Dead);
  EXPECT_EQ(f1->pending_result, 1);
  EXPECT_EQ(f2->pending_result, 9);
}

TEST_F(VMTest, YieldFiber) {
  auto chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(1, 2, 3);
  append_ins(chunk, {Push(0), Yield{}, Push(1), Yield{}, Push(2), Return{}});
  Fiber* f = vm.AddFiber(chunk);

  f->state = FiberState::Running;
  std::ignore = vm.Run();
  EXPECT_EQ(f->state, FiberState::Suspended);
  EXPECT_EQ(f->pending_result, 1);

  f->state = FiberState::Running;
  std::ignore = vm.Run();
  EXPECT_EQ(f->state, FiberState::Suspended);
  EXPECT_EQ(f->pending_result, 2);

  f->state = FiberState::Running;
  std::ignore = vm.Run();
  EXPECT_EQ(f->state, FiberState::Dead);
  EXPECT_EQ(f->pending_result, 3);
}

TEST_F(VMTest, SpawnFiber) {
  int call_count = 0;
  std::vector<Value> arg;
  std::unordered_map<std::string, Value> kwarg;
  auto* fn = gc.Allocate<NativeFunction>(
      "my_function", [&](Fiber& f, std::vector<Value> args,
                         std::unordered_map<std::string, Value> kwargs) {
        ++call_count;
        arg = std::move(args);
        kwarg = std::move(kwargs);
        return Value();
      });

  auto* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(fn, 1, "foo", "boo");
  append_ins(chunk, {Push{0}, Push{1}, Push{2}, Push{3},
                     // fn, 1, "foo", "boo"
                     MakeFiber{1, 1}, Push{1}, Return{}});
  std::ignore = run_and_get(chunk);
  EXPECT_EQ(call_count, 1);
  ASSERT_EQ(arg.size(), 1);
  EXPECT_EQ(arg.front(), 1);
  ASSERT_EQ(kwarg.size(), 1);
  ASSERT_TRUE(kwarg.contains("foo"));
  EXPECT_EQ(kwarg["foo"], "boo");
}

TEST_F(VMTest, AwaitFiber) {
  Code* print_code = gc.Allocate<Code>();
  print_code->const_pool = value_vector("print", std::monostate());
  append_ins(print_code, {Push{1}, Yield{}, LoadLocal{1},
                          Return{}});  // yield nil; return arg0;
  Function* print_fn = gc.Allocate<Function>(print_code, 0, 1);

  Code* chunk = gc.Allocate<Code>();
  chunk->const_pool = value_vector(print_fn, "foo", "boo", std::monostate());
  append_ins(chunk, {
                        Push{0}, Push{1}, MakeFiber{.argcnt = 1, .kwargcnt = 0},
                        // handle1 = print_corout("foo");
                        Push{0}, Push{2}, MakeFiber{.argcnt = 1, .kwargcnt = 0},
                        // handle2 = print_corout("boo");
                        Await{}, Pop{},  // await handle2;
                        Dup{}, Await{}, Pop{}, Await{},
                        Return{}  // await handle1; return await handle1;
                    });

  Value result = run_and_get(chunk);
  EXPECT_EQ(result, "foo");
}

TEST_F(VMTest, FunctionGlobal) {
  Code* code = gc.Allocate<Code>();
  code->const_pool = value_vector("one");
  append_ins(code, {LoadLocal{1}, LoadGlobal{0}, BinaryOp(Op::Add), Return{}});
  Function* fn = gc.Allocate<Function>(code, 0, 1);
  fn->globals = gc.Allocate<Dict>(
      std::unordered_map<std::string, Value>{{"one", Value("one")}});

  Code* chunk = gc.Allocate<Code>();
  chunk->const_pool =
      value_vector(fn, code, "one", 1, "arg1", "first", "second");
  append_ins(chunk,
             {Push{3}, StoreGlobal{2},  // global: one = 1
              Push{5}, Push{0}, Push{2}, Call{.argcnt = 1, .kwargcnt = 0},
              // first: fn("one") -> "one" + "one";
              // function global: one = "one"
              Push{6}, Push{1}, Push{4}, MakeFunction{0, 1, 0, false, false},
              Push{3}, Call{1, 0},
              // second: fn(1) -> 1 + 1;
              // function global is current namespace
              MakeDict{.nelms = 2}, Return{}});

  Value result = run_and_get(chunk);
  EXPECT_EQ(result.Str(), "{second:2,first:oneone}");
}

}  // namespace serilang_test
