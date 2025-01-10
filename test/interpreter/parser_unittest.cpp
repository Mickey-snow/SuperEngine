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

#include "util.hpp"

#include "base/expr_ast.hpp"
#include "interpreter/parser.hpp"

TEST(ExpressionParserTest, Basic) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(1), tok::Plus(), tok::Int(2));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "1+2");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(3), tok::Minus(), tok::Int(4));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "3-4");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(5), tok::Mult(), tok::Int(6));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "5*6");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input = TokenArray(tok::Int(7), tok::Div(), tok::Int(8));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "7/8");
  }
}

struct GetOp {
  Op operator()(const auto& x) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::same_as<T, BinaryExpr>)
      return x.op;
    return Op::Unknown;
  }
};

TEST(ExpressionParserTest, Precedence) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input = TokenArray(tok::Int(5), tok::Mult(), tok::Int(6),
                                          tok::Plus(), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Visit(GetOp()), Op::Add);
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input = TokenArray(tok::Int(5), tok::Plus(), tok::Int(6),
                                          tok::Div(), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Visit(GetOp()), Op::Add);
  }
}

TEST(ExpressionParserTest, Parenthesis) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input =
      TokenArray(tok::ParenthesisL(), tok::Int(5), tok::Plus(), tok::Int(6),
                 tok::ParenthesisR(), tok::Div(), tok::Int(7));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Visit(GetOp()), Op::Div);
  EXPECT_EQ(result->DebugString(), "(5+6)/7");
}
