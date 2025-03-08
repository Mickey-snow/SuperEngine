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

#include "m6/op.hpp"
#include "machine/instruction.hpp"
#include "machine/rlmachine.hpp"
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

using namespace m6;

class VMTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_NO_THROW({
      machine = std::make_shared<RLMachine>(nullptr, nullptr, ScriptLocation(),
                                            nullptr);
    });
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
    (machine->operator()(std::forward<Ts>(params)), ...);
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
}
