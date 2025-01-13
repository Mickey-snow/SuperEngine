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

#include "base/expr_ast.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/tokenizer.hpp"

#include <algorithm>

class inf2prefTest : public ::testing::Test {
 protected:
  std::string ToPrefix(std::string_view infix_expr) {
    Tokenizer tokenizer(infix_expr, false);
    EXPECT_NO_THROW(tokenizer.Parse());

    std::vector<Token> tokens;
    std::copy_if(tokenizer.parsed_tok_.cbegin(), tokenizer.parsed_tok_.cend(),
                 std::back_inserter(tokens), [](const Token& t) {
                   return !std::holds_alternative<tok::WS>(t);
                 });

    std::shared_ptr<ExprAST> ast = nullptr;
    EXPECT_NO_THROW(ast = ParseExpression(std::span(tokens)));

    static const GetPrefix get_prefix_visitor;
    return ast ? ast->Apply(get_prefix_visitor) : "???";
  }
};

TEST_F(inf2prefTest, NestedArithmetic) {
  constexpr std::string_view input =
      "a + b * (c - d) / e << f && g || h == i != j";
  EXPECT_EQ(ToPrefix(input), "|| && << + a / * b - c d e f g != == h i j");
}

TEST_F(inf2prefTest, MultiAssignments) {
  // constexpr std::string_view input = "x += y & (z | w) ^ (u << v) >>= t";
  // EXPECT_EQ(ToPrefix(input), "|| && << + a / * b - c d e f g != == h i j");
}
