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

struct GetPrefix {
  std::string operator()(const BinaryExpr& x) const {
    return ToString(x.op) + ' ' + x.lhs->Apply(*this) + ' ' +
           x.rhs->Apply(*this);
  }
  std::string operator()(const UnaryExpr& x) const {
    return ToString(x.op) + ' ' + x.sub->Apply(*this);
  }
  std::string operator()(const ParenExpr& x) const {
    return x.sub->Apply(*this);
  }
  std::string operator()(const ReferenceExpr& x) const {
    return x.id + '[' + x.idx->Apply(*this) + ']';
  }
  std::string operator()(std::monostate) const { return "<null>"; }
  std::string operator()(int x) const { return std::to_string(x); }
  std::string operator()(const std::string& str) const { return str; }
} get_prefix_visitor;

TEST(ExpressionParserTest, Precedence) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input = TokenArray(tok::Int(5), tok::Mult(), tok::Int(6),
                                          tok::Plus(), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor), "+ * 5 6 7");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input = TokenArray(tok::Int(5), tok::Plus(), tok::Int(6),
                                          tok::Div(), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor), "+ 5 / 6 7");
  }
}

TEST(ExpressionParserTest, Parenthesis) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input =
      TokenArray(tok::ParenthesisL(), tok::Int(5), tok::Plus(), tok::Int(6),
                 tok::ParenthesisR(), tok::Div(), tok::Int(7));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), "/ + 5 6 7");
}

TEST(ExpressionParserTest, ExprList) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(tok::Int(5), tok::Plus(), tok::Int(6),
                                        tok::Comma(), tok::Int(8), tok::Comma(),
                                        tok::Int(9), tok::Div(), tok::Int(7));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), ", , + 5 6 8 / 9 7");
}

TEST(ExpressionParserTest, Identifier) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input =
      TokenArray(tok::ID("v1"), tok::Plus(), tok::ID("v2"), tok::Div(),
                 tok::ID("v3"), tok::SquareL(), tok::ID("v4"), tok::Plus(),
                 tok::ID("v5"), tok::SquareR());

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), "+ v1 / v2 v3[+ v4 v5]");
}
