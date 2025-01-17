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

#include "core/expr_ast.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/tokenizer.hpp"

#include <algorithm>

TEST(FormulaParserTest, InfixToPrefix) {
  struct TestCase {
    std::string input;
    std::string expected_prefix_str;
  };

  auto test_cases = std::vector<TestCase>{
      TestCase{"a + b * (c - d) / e << f && g || h == i != j",
               "|| && << + a / * b - c d e f g != == h i j"},
      TestCase{"x += y & (z | w) ^ (u << v) >>= t",
               "+= x >>= ^ & y | z w << u v t"},
      TestCase{"array1[array2[index1 + index2] * (index3 - index4)] = value",
               "= array1[* array2[+ index1 index2] - index3 index4] value"},
      TestCase{"~a + -b * +c - (d && e) || f", "|| - + ~ a * - b + c && d e f"},
      TestCase{"(a <= b) && (c > d) || (e == f) && (g != h)",
               "|| && <= a b > c d && == e f != g h"},
      TestCase{"result = a * (b + c) - d / e += f << g",
               "= result += - * a + b c / d e << f g"},
      TestCase{"data[index1] += (temp - buffer[i] * factor[j]) >> shift",
               "+= data[index1] >> - temp * buffer[i] factor[j] shift"},
      TestCase{"a + b * c - d / e % f & g | h ^ i << j >> k",
               "| & - + a * b c % / d e f g ^ h >> << i j k"},
      TestCase{"array[i += 2] *= (k[j -= 3] /= 4) + l",
               "*= array[+= i 2] + /= k[-= j 3] 4 l"},
      TestCase{"data[array1[index] << 2] = value",
               "= data[<< array1[index] 2] value"},
      TestCase{"final_result = ((a + b) * (c - d) / e) << (f & g) | (h ^ ~i) "
               "&& j || k == l != m <= n >= o < p > q",
               "= final_result || && | << / * + a b - c d e & f g ^ h ~ i j "
               "!= == k l > < >= <= m n o p q"}};

  for (const auto& it : test_cases) {
    Tokenizer tokenizer(it.input, false);
    EXPECT_NO_THROW(tokenizer.Parse());

    std::vector<Token> tokens;
    std::copy_if(tokenizer.parsed_tok_.cbegin(), tokenizer.parsed_tok_.cend(),
                 std::back_inserter(tokens), [](const Token& t) {
                   return !std::holds_alternative<tok::WS>(t);
                 });

    std::shared_ptr<ExprAST> ast = nullptr;
    EXPECT_NO_THROW(ast = ParseExpression(std::span(tokens)));

    static const GetPrefix get_prefix_visitor;
    std::string prefix = ast ? ast->Apply(get_prefix_visitor) : "???";
    EXPECT_EQ(prefix, it.expected_prefix_str);
  }
}

class ExprEvaluatorTest : public ::testing::Test {
 protected:
  int Eval(std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));

    static const Evaluator evaluator;
    return expr->Apply(evaluator);
  }
};

TEST_F(ExprEvaluatorTest, EvalUnary) {
  EXPECT_EQ(Eval("+1"), 1);
  EXPECT_EQ(Eval("-2"), -2);
  EXPECT_EQ(Eval("~25"), -26);
  EXPECT_EQ(Eval("+0"), 0);
  EXPECT_EQ(Eval("-0"), 0);
  EXPECT_EQ(Eval("~ -1"), 0);
}

TEST_F(ExprEvaluatorTest, EvalBinary) {
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

TEST_F(ExprEvaluatorTest, EvalParentheses) {
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

TEST_F(ExprEvaluatorTest, EvalComplexExpressions) {
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
}
