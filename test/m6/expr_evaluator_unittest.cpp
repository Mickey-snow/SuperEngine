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

#include "m6/evaluator.hpp"
#include "m6/op.hpp"
#include "m6/parser.hpp"
#include "m6/tokenizer.hpp"
#include "m6/value.hpp"
#include "m6/value_error.hpp"

#include "util.hpp"

using namespace m6;

class ExpressionEvaluatorTest : public ::testing::Test {
 protected:
  auto Eval(const std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    static Evaluator evaluator;
    return expr->Apply(evaluator);
  }
};

TEST_F(ExpressionEvaluatorTest, Unary) {
  EXPECT_VALUE_EQ(Eval("+1"), 1);
  EXPECT_VALUE_EQ(Eval("-2"), -2);
  EXPECT_VALUE_EQ(Eval("~25"), -26);
  EXPECT_VALUE_EQ(Eval("+0"), 0);
  EXPECT_VALUE_EQ(Eval("-0"), 0);
  EXPECT_VALUE_EQ(Eval("~ -1"), 0);
}

TEST_F(ExpressionEvaluatorTest, Binary) {
  // Addition
  EXPECT_VALUE_EQ(Eval("1 + 1"), 2);
  EXPECT_VALUE_EQ(Eval("2 + 3"), 5);
  EXPECT_VALUE_EQ(Eval(" -5 + 10 "), 5);

  // Subtraction
  EXPECT_VALUE_EQ(Eval("10 - 4"), 6);
  EXPECT_VALUE_EQ(Eval("-2 - (-3)"), 1);

  // Multiplication
  EXPECT_VALUE_EQ(Eval("3 * 4"), 12);
  EXPECT_VALUE_EQ(Eval("-2 * 5"), -10);

  // Division
  EXPECT_VALUE_EQ(Eval("10 / 2"), 5);
  EXPECT_VALUE_EQ(Eval("7 / 3"), 2);  // currently, integer division
  EXPECT_VALUE_EQ(Eval("0 / 0"), 0)
      << "special case: division by zero should results 0.";

  // Modulo
  EXPECT_VALUE_EQ(Eval("10 % 3"), 1);
  EXPECT_VALUE_EQ(Eval("-10 % 3"), -1);

  // Bitwise AND
  EXPECT_VALUE_EQ(Eval("5 & 3"), 1);
  EXPECT_VALUE_EQ(Eval("12 & 5"), 4);

  // Bitwise OR
  EXPECT_VALUE_EQ(Eval("5 | 3"), 7);
  EXPECT_VALUE_EQ(Eval("12 | 5"), 13);

  // Bitwise XOR
  EXPECT_VALUE_EQ(Eval("5 ^ 3"), 6);
  EXPECT_VALUE_EQ(Eval("12 ^ 5"), 9);

  // Bitwise Shifts
  EXPECT_VALUE_EQ(Eval("1 << 3"), 8);
  EXPECT_VALUE_EQ(Eval("16 >> 2"), 4);
  EXPECT_VALUE_EQ(Eval("5 >>> 2"), 1);
  EXPECT_VALUE_EQ(Eval("-5 >>> 2"), 1073741822);
  EXPECT_THROW(Eval("1 >> -1"), ValueError);
  EXPECT_THROW(Eval("1 << -1"), ValueError);
  EXPECT_THROW(Eval("1 >>> -1"), ValueError);

  // Comparison Operators
  EXPECT_VALUE_EQ(Eval("5 == 5"), 1);
  EXPECT_VALUE_EQ(Eval("5 != 3"), 1);
  EXPECT_VALUE_EQ(Eval("5 < 10"), 1);
  EXPECT_VALUE_EQ(Eval("10 <= 10"), 1);
  EXPECT_VALUE_EQ(Eval("15 > 10"), 1);
  EXPECT_VALUE_EQ(Eval("10 >= 15"), 0);

  // Logical AND
  EXPECT_VALUE_EQ(Eval("1 && 1"), 1);
  EXPECT_VALUE_EQ(Eval("1 && 0"), 0);
  EXPECT_VALUE_EQ(Eval("0 && 0"), 0);

  // Logical OR
  EXPECT_VALUE_EQ(Eval("1 || 0"), 1);
  EXPECT_VALUE_EQ(Eval("0 || 0"), 0);
  EXPECT_VALUE_EQ(Eval("0 || 1"), 1);
}

TEST_F(ExpressionEvaluatorTest, Parentheses) {
  // Simple parentheses
  EXPECT_VALUE_EQ(Eval("(1 + 2)"), 3);
  EXPECT_VALUE_EQ(Eval("-(3)"), -3);

  // Nested parentheses
  EXPECT_VALUE_EQ(Eval("((2 + 3) * 4)"), 20);
  EXPECT_VALUE_EQ(Eval("-( (1 + 2) * (3 + 4) )"), -21);

  // Multiple parentheses
  EXPECT_VALUE_EQ(Eval("(1 + (2 * (3 + 4)))"), 15);
  EXPECT_VALUE_EQ(Eval("((1 + 2) * (3 + (4 * 5)))"), 69);
}

TEST_F(ExpressionEvaluatorTest, ComplexExpressions) {
  // Combining multiple operators with precedence
  EXPECT_VALUE_EQ(Eval("1 + 2 * 3"), 7);  // 2*3 +1
  EXPECT_VALUE_EQ(Eval("(1 + 2) * 3"), 9);
  EXPECT_VALUE_EQ(Eval("4 + 5 * 6 / 3 - 2"), 12);  // 5*6=30 /3=10 +4=14 -2=12

  // Logical and bitwise combinations
  EXPECT_VALUE_EQ(Eval("1 + 2 && 3 | 4"), 1);  // 1+2=3; 3|4=7; 3&7=1
  EXPECT_VALUE_EQ(Eval("~(1 << 2)"), -5);
  EXPECT_VALUE_EQ(Eval("3 + ~2 * 2"), -3);  // 3 -6 = -3

  // Mixed unary and binary
  EXPECT_VALUE_EQ(Eval("-3 + +2"), -1);
  EXPECT_VALUE_EQ(Eval("~1 + 2"), 0);

  // Complex Arithmetic
  EXPECT_VALUE_EQ(
      Eval("((3 + 5) * (2 - 8)) / ((4 % 3) + (7 << 2)) - ~(15 & 3) | (12 "
           "^ 5) && (9 > 3)"),
      1);

  EXPECT_VALUE_EQ(
      Eval("( ( (1 + 2) * (3 + 4) ) / (5 - (6 / (7 + 8))) ) + (9 << (2 + "
           "3)) - ~(4 | 2)"),
      299);

  EXPECT_VALUE_EQ(
      Eval("(((1 + 2) * (3 - 4) / (5 % 2)) << (6 & 3)) | ((7 ^ 8) && (9 "
           "> 10)) - ~11"),
      -4);
}

TEST_F(ExpressionEvaluatorTest, StringArithmetic) {
  EXPECT_VALUE_EQ(Eval(R"("Hello, " + "World!")"), "Hello, World!");
  EXPECT_VALUE_EQ(Eval(R"(("Hi! " + "There! ") * 2)"),
                  "Hi! There! Hi! There! ");
  EXPECT_VALUE_EQ(Eval(R"((("Hello" + ", ") * 2) + ("World" + "!") * 1)"),
                  "Hello, Hello, World!");
  EXPECT_VALUE_EQ(Eval(R"("" + "Non-empty" + "")"), "Non-empty");
  EXPECT_VALUE_EQ(Eval(R"("nothing" * (3-3))"), "");
  EXPECT_VALUE_EQ(Eval(R"(("Math" + ("+" * 2)) * (1 + 1) == "Math++Math++")"),
                  1);

  EXPECT_THROW(Eval(R"("Error" * "3")"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Number: " + 100)"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Invalid" - "Operation")"), m6::UndefinedOperator);
  EXPECT_THROW(Eval(R"("Negative" * -2)"), m6::UndefinedOperator);
}
