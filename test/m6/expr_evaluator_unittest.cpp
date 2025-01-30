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

#include "m6/expr_ast.hpp"
#include "m6/parser.hpp"
#include "m6/tokenizer.hpp"

using namespace m6;

class ExpressionEvaluatorTest : public ::testing::Test {
 protected:
  int Eval(const std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    static Evaluator evaluator;
    return expr->Apply(evaluator);
  }
};

TEST_F(ExpressionEvaluatorTest, BasicArithmetic) {
  {
    constexpr std::string_view input =
        "((3 + 5) * (2 - 8)) / ((4 % 3) + (7 << 2)) - ~(15 & 3) | (12 ^ 5) && "
        "(9 "
        "> 3)";
    EXPECT_TRUE(Eval(input));
  }

  {
    constexpr std::string_view input =
        "( ( (1 + 2) * (3 + 4) ) / (5 - (6 / (7 + 8))) ) + (9 << (2 + 3)) - "
        "~(4 | 2)";
    EXPECT_EQ(Eval(input), 299);
  }

  {
    constexpr std::string_view input =
        "(((1 + 2) * (3 - 4) / (5 % 2)) << (6 & 3)) | ((7 ^ 8) && (9 > 10)) - "
        "~11";
    EXPECT_EQ(Eval(input), -4);
  }
}
