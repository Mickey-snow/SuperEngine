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

#include "m6/compiler.hpp"
#include "m6/exception.hpp"
#include "m6/parser.hpp"
#include "m6/tokenizer.hpp"
#include "machine/op.hpp"
#include "machine/rlmachine.hpp"
#include "machine/value.hpp"

namespace m6test {
using namespace m6;

class ExpressionCompilerTest : public ::testing::Test {
 protected:
  ExpressionCompilerTest()
      : machine(std::make_shared<RLMachine>(nullptr, nullptr, nullptr)),
        stack(const_cast<std::vector<Value>&>(machine->GetStack())) {}

  auto Eval(const std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    auto instructions = Compiler().Compile(expr);
    for (const auto& it : instructions)
      std::visit(*machine, it);

    if (stack.empty()) {
      ADD_FAILURE() << "stack underflow.";
      return Value(std::monostate());
    } else {
      Value ret = stack.back();
      stack.pop_back();
      return ret;
    }
  }

  std::shared_ptr<RLMachine> machine;
  std::vector<Value>& stack;
};

TEST_F(ExpressionCompilerTest, Unary) {
  EXPECT_EQ(Eval("+1"), 1);
  EXPECT_EQ(Eval("-2"), -2);
  EXPECT_EQ(Eval("~25"), -26);
  EXPECT_EQ(Eval("+0"), 0);
  EXPECT_EQ(Eval("-0"), 0);
  EXPECT_EQ(Eval("~ -1"), 0);
}

TEST_F(ExpressionCompilerTest, Binary) {
  // Addition
  EXPECT_EQ(Eval("1 + 1"), 2);
  EXPECT_EQ(Eval("2 + 3"), 5);
  EXPECT_EQ(Eval(" -5 + 10 "), 5);

  // Subtraction
  EXPECT_EQ(Eval("10 - 4"), 6);
  EXPECT_EQ(Eval("-2 - (-3)"), 1);

  // Multiplication
  EXPECT_EQ(Eval("3 * 4"), 12);
  EXPECT_EQ(Eval("-2 * 5"), -10);

  // Division
  EXPECT_EQ(Eval("10 / 2"), 5);
  EXPECT_EQ(Eval("7 / 3"), 2);  // currently, integer division
  EXPECT_EQ(Eval("0 / 0"), 0)
      << "special case: division by zero should results 0.";

  // Modulo
  EXPECT_EQ(Eval("10 % 3"), 1);
  EXPECT_EQ(Eval("-10 % 3"), -1);

  // Bitwise AND
  EXPECT_EQ(Eval("5 & 3"), 1);
  EXPECT_EQ(Eval("12 & 5"), 4);

  // Bitwise OR
  EXPECT_EQ(Eval("5 | 3"), 7);
  EXPECT_EQ(Eval("12 | 5"), 13);

  // Bitwise XOR
  EXPECT_EQ(Eval("5 ^ 3"), 6);
  EXPECT_EQ(Eval("12 ^ 5"), 9);

  // Bitwise Shifts
  EXPECT_EQ(Eval("1 << 3"), 8);
  EXPECT_EQ(Eval("16 >> 2"), 4);
  EXPECT_EQ(Eval("5 >>> 2"), 1);
  EXPECT_EQ(Eval("-5 >>> 2"), 1073741822);
  EXPECT_THROW(Eval("1 >> -1"), ValueError);
  EXPECT_THROW(Eval("1 << -1"), ValueError);
  EXPECT_THROW(Eval("1 >>> -1"), ValueError);

  // Comparison Operators
  EXPECT_EQ(Eval("5 == 5"), 1);
  EXPECT_EQ(Eval("5 != 3"), 1);
  EXPECT_EQ(Eval("5 < 10"), 1);
  EXPECT_EQ(Eval("10 <= 10"), 1);
  EXPECT_EQ(Eval("15 > 10"), 1);
  EXPECT_EQ(Eval("10 >= 15"), 0);

  // Logical AND
  EXPECT_EQ(Eval("1 && 1"), 1);
  EXPECT_EQ(Eval("1 && 0"), 0);
  EXPECT_EQ(Eval("0 && 0"), 0);

  // Logical OR
  EXPECT_EQ(Eval("1 || 0"), 1);
  EXPECT_EQ(Eval("0 || 0"), 0);
  EXPECT_EQ(Eval("0 || 1"), 1);
}

TEST_F(ExpressionCompilerTest, Parentheses) {
  // Simple parentheses
  EXPECT_EQ(Eval("(1 + 2)"), 3);
  EXPECT_EQ(Eval("-(3)"), -3);

  // Nested parentheses
  EXPECT_EQ(Eval("((2 + 3) * 4)"), 20);
  EXPECT_EQ(Eval("-( (1 + 2) * (3 + 4) )"), -21);

  // Multiple parentheses
  EXPECT_EQ(Eval("(1 + (2 * (3 + 4)))"), 15);
  EXPECT_EQ(Eval("((1 + 2) * (3 + (4 * 5)))"), 69);
}

TEST_F(ExpressionCompilerTest, ComplexExpressions) {
  // Combining multiple operators with precedence
  EXPECT_EQ(Eval("1 + 2 * 3"), 7);  // 2*3 +1
  EXPECT_EQ(Eval("(1 + 2) * 3"), 9);
  EXPECT_EQ(Eval("4 + 5 * 6 / 3 - 2"), 12);  // 5*6=30 /3=10 +4=14 -2=12

  // Logical and bitwise combinations
  EXPECT_EQ(Eval("1 + 2 && 3 | 4"), 1);  // 1+2=3; 3|4=7; 3&7=1
  EXPECT_EQ(Eval("~(1 << 2)"), -5);
  EXPECT_EQ(Eval("3 + ~2 * 2"), -3);  // 3 -6 = -3

  // Mixed unary and binary
  EXPECT_EQ(Eval("-3 + +2"), -1);
  EXPECT_EQ(Eval("~1 + 2"), 0);

  // Complex Arithmetic
  EXPECT_EQ(Eval("((3 + 5) * (2 - 8)) / ((4 % 3) + (7 << 2)) - ~(15 & 3) | (12 "
                 "^ 5) && (9 > 3)"),
            1);

  EXPECT_EQ(Eval("( ( (1 + 2) * (3 + 4) ) / (5 - (6 / (7 + 8))) ) + (9 << (2 + "
                 "3)) - ~(4 | 2)"),
            299);

  EXPECT_EQ(Eval("(((1 + 2) * (3 - 4) / (5 % 2)) << (6 & 3)) | ((7 ^ 8) && (9 "
                 "> 10)) - ~11"),
            -4);
}

TEST_F(ExpressionCompilerTest, StringArithmetic) {
  EXPECT_EQ(Eval(R"("Hello, " + "World!")"), "Hello, World!");
  EXPECT_EQ(Eval(R"(("Hi! " + "There! ") * 2)"), "Hi! There! Hi! There! ");
  EXPECT_EQ(Eval(R"((("Hello" + ", ") * 2) + ("World" + "!") * 1)"),
            "Hello, Hello, World!");
  EXPECT_EQ(Eval(R"("" + "Non-empty" + "")"), "Non-empty");
  EXPECT_EQ(Eval(R"("nothing" * (3-3))"), "");
  EXPECT_EQ(Eval(R"(("Math" + ("+" * 2)) * (1 + 1) == "Math++Math++")"), 1);

  EXPECT_THROW(Eval(R"("Error" * "3")"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Number: " + 100)"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Invalid" - "Operation")"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Negative" * -2)"), m6::UndefinedOperator);
}

}  // namespace m6test
