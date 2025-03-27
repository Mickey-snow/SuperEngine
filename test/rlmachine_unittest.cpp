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

#include "machine/instruction.hpp"
#include "machine/op.hpp"
#include "machine/rlmachine.hpp"
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

using namespace m6;

class VMTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_NO_THROW(
        { machine = std::make_shared<RLMachine>(nullptr, nullptr, nullptr); });
    ASSERT_NE(machine, nullptr);
  }

  std::shared_ptr<RLMachine> machine;

  std::string DescribeStack() const {
    return Join(", ", std::views::all(machine->GetStack()) |
                          std::views::transform(
                              [](Value const& x) { return x.Desc(); }));
  }

  template <typename... Ts>
    requires(std::convertible_to<Ts, Instruction> && ...)
  void Execute(Ts&&... params) {
    std::vector<Instruction> instructions{std::forward<Ts>(params)...};
    machine->halted_ = false;
    machine->ip_ = 0;
    machine->script_ = std::span(instructions);
    machine->Execute();
  }
};

TEST_F(VMTest, init) {
  machine->operator()(End());
  EXPECT_TRUE(machine->IsHalted());
  EXPECT_EQ(DescribeStack(), "");
}

TEST_F(VMTest, StackManipulation) {
  Execute(Push(Value(123)), Push(Value("hello")), Push(Value("world")));
  EXPECT_EQ(DescribeStack(), "<int: 123>, <str: hello>, <str: world>");
  Execute(Pop(2));
  EXPECT_EQ(DescribeStack(), "<int: 123>");
}

TEST_F(VMTest, Operation) {
  Execute(Push(Value(123)), Push(Value(3)), Push(Value(92)), BinaryOp(Op::Mul),
          BinaryOp(Op::Add));
  EXPECT_EQ(DescribeStack(), "<int: 399>");

  machine->stack_.clear();
  Execute(Push(Value("hello, ")), Push(Value("world")), BinaryOp(Op::Add),
          Push(Value(3)), BinaryOp(Op::Mul));
  EXPECT_EQ(DescribeStack(), "<str: hello, worldhello, worldhello, world>");
}

TEST_F(VMTest, Load) {
  Execute(Push(Value(123)), Push(Value("Hello")), Load(1));
  EXPECT_EQ(DescribeStack(), "<int: 123>, <str: Hello>, <str: Hello>")
      << "Should copy the second element to stack top";
}

TEST_F(VMTest, Store) {
  Execute(Push(Value(123)), Push(Value("Hello")), Store(0));
  EXPECT_EQ(DescribeStack(), "<str: Hello>, <str: Hello>")
      << "Should copy the element at top of the stack to the first location";
}

TEST_F(VMTest, Jump) {
  // jump if true
  Execute(Push(Value(-10)), Load(0), Push(Value(1)), BinaryOp(Op::Add),
          Store(0), Jt(-5));
  EXPECT_EQ(DescribeStack(), "<int: 0>");

  // unconditional jump
  machine->stack_.clear();
  Execute(Jmp(1), Push(Value(0)), Push(Value(1)), Push(Value(2)));
  EXPECT_EQ(DescribeStack(), "<int: 1>, <int: 2>");

  // jump if false
  machine->stack_.clear();
  Execute(Push(Value("")), Jf(1), Push(Value(0)), Push(Value(1)));
  EXPECT_EQ(DescribeStack(), "<int: 1>");
}
