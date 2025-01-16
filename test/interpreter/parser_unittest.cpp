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

inline static GetPrefix get_prefix_visitor;

TEST(ExprastParserTest, BasicArithmetic) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(1), tok::Operator(Op::Add), tok::Int(2));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "1+2");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(3), tok::Operator(Op::Sub), tok::Int(4));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "3-4");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(5), tok::Operator(Op::Mul), tok::Int(6));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "5*6");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(7), tok::Operator(Op::Div), tok::Int(8));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "7/8");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(9), tok::Operator(Op::Mod), tok::Int(10));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->DebugString(), "9%10");
  }
}

TEST(ExprastParserTest, Precedence) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(5), tok::Operator(Op::Mul), tok::Int(6),
                   tok::Operator(Op::Add), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor), "+ * 5 6 7");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::Int(5), tok::Operator(Op::Add), tok::Int(6),
                   tok::Operator(Op::Div), tok::Int(7));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor), "+ 5 / 6 7");
  }
}

TEST(ExprastParserTest, Parenthesis) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(
      tok::ParenthesisL(), tok::Int(5), tok::Operator(Op::Add), tok::Int(6),
      tok::ParenthesisR(), tok::Operator(Op::Div), tok::Int(7));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), "/ + 5 6 7");
}

TEST(ExprastParserTest, ExprList) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(
      tok::Int(5), tok::Operator(Op::Add), tok::Int(6),
      tok::Operator(Op::Comma), tok::Int(8), tok::Operator(Op::Comma),
      tok::Int(9), tok::Operator(Op::Div), tok::Int(7));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), ", , + 5 6 8 / 9 7");
}

TEST(ExprastParserTest, Identifier) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(
      tok::ID("v1"), tok::Operator(Op::Add), tok::ID("v2"),
      tok::Operator(Op::Div), tok::ID("v3"), tok::SquareL(), tok::ID("v4"),
      tok::Operator(Op::Add), tok::ID("v5"), tok::SquareR());

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor), "+ v1 / v2 v3[+ v4 v5]");
}

TEST(ExprastParserTest, Comparisons) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(
      tok::ID("v1"), tok::Operator(Op::Equal), tok::ID("v2"),
      tok::Operator(Op::NotEqual), tok::ID("v3"), tok::Operator(Op::Greater),
      tok::ID("v4"), tok::Operator(Op::Less), tok::ID("v5"),
      tok::Operator(Op::LessEqual), tok::Int(12),
      tok::Operator(Op::GreaterEqual), tok::Int(13));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor),
            "!= == v1 v2 >= <= < > v3 v4 v5 12 13");
}

TEST(ExprastParserTest, Shifts) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input = TokenArray(
      tok::ID("v1"), tok::Operator(Op::ShiftLeft), tok::ID("v2"),
      tok::Operator(Op::Less), tok::ID("v3"), tok::Operator(Op::ShiftRight),
      tok::ID("v4"), tok::Operator(Op::Add), tok::ID("v5"),
      tok::Operator(Op::ShiftLeft), tok::Int(12), tok::Operator(Op::Less),
      tok::Int(13));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor),
            "< < << v1 v2 << >> v3 + v4 v5 12 13");
}

TEST(ExprastParserTest, Logical) {
  std::shared_ptr<ExprAST> result = nullptr;
  std::vector<Token> input =
      TokenArray(tok::ID("v1"), tok::Operator(Op::LogicalOr), tok::ID("v2"),
                 tok::Operator(Op::LogicalAnd), tok::ID("v3"),
                 tok::Operator(Op::ShiftRight), tok::ID("v4"),
                 tok::Operator(Op::LogicalOr), tok::ID("v5"),
                 tok::Operator(Op::LogicalAnd), tok::Int(12));

  ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Apply(get_prefix_visitor),
            "|| || v1 && v2 >> v3 v4 && v5 12");
}

TEST(ExprastParserTest, Assignment) {
  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::ID("v1"), tok::Operator(Op::Assign), tok::ID("v2"),
                   tok::Operator(Op::ShiftLeftAssign), tok::ID("v3"),
                   tok::Operator(Op::ShiftRight), tok::ID("v4"),
                   tok::Operator(Op::ShiftRightAssign), tok::ID("v5"),
                   tok::Operator(Op::LogicalAnd), tok::Int(12),
                   tok::Operator(Op::AddAssign), tok::ID("v2"),
                   tok::Operator(Op::SubAssign), tok::ID("v3"),
                   tok::Operator(Op::MulAssign), tok::ID("v4"),
                   tok::Operator(Op::ModAssign), tok::ID("v5"));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor),
              "= v1 <<= v2 >>= >> v3 v4 += && v5 12 -= v2 *= v3 %= v4 v5");
  }

  {
    std::shared_ptr<ExprAST> result = nullptr;
    std::vector<Token> input =
        TokenArray(tok::ID("v1"), tok::Operator(Op::BitOrAssign), tok::ID("v2"),
                   tok::Operator(Op::ShiftLeft), tok::ID("v3"),
                   tok::Operator(Op::ShiftRight), tok::ID("v4"),
                   tok::Operator(Op::BitXorAssign), tok::ID("v5"),
                   tok::Operator(Op::LogicalAnd), tok::Int(12),
                   tok::Operator(Op::BitAndAssign), tok::ID("v2"));

    ASSERT_NO_THROW(result = ParseExpression(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Apply(get_prefix_visitor),
              "|= v1 ^= >> << v2 v3 v4 &= && v5 12 v2");
  }
}

TEST(ExprastParserTest, BitwiseOperators) {
  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "& a b");
  }

  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::BitOr), tok::ID("b"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "| a b");
  }

  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::BitXor), tok::ID("b"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "^ a b");
  }

  {
    // a & b | c ^ d
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b"),
                   tok::Operator(Op::BitOr), tok::ID("c"),
                   tok::Operator(Op::BitXor), tok::ID("d"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "| & a b ^ c d");
  }
}

TEST(ExprastParserTest, UnaryOperators) {
  {
    std::vector<Token> input = TokenArray(tok::Operator(Op::Sub), tok::ID("a"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "- a");
  }

  {
    std::vector<Token> input = TokenArray(tok::Operator(Op::Add), tok::ID("a"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "+ a");
  }

  {
    std::vector<Token> input =
        TokenArray(tok::Operator(Op::Tilde), tok::ID("a"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "~ a");
  }

  {
    std::vector<Token> input = TokenArray(
        tok::Operator(Op::Sub), tok::Operator(Op::Tilde), tok::ID("a"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "- ~ a");
  }

  {
    std::vector<Token> input =
        TokenArray(tok::Operator(Op::Sub), tok::ParenthesisL(), tok::ID("a"),
                   tok::Operator(Op::Add), tok::ID("b"), tok::ParenthesisR());
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "- + a b");
  }
}

TEST(ExprastParserTest, MixedPrecedence) {
  // a + b * c
  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::Add), tok::ID("b"),
                   tok::Operator(Op::Mul), tok::ID("c"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "+ a * b c");
  }

  // a & b | c ^ d
  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b"),
                   tok::Operator(Op::BitOr), tok::ID("c"),
                   tok::Operator(Op::BitXor), tok::ID("d"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "| & a b ^ c d");
  }

  // -a + b * ~c
  {
    std::vector<Token> input =
        TokenArray(tok::Operator(Op::Sub), tok::ID("a"), tok::Operator(Op::Add),
                   tok::ID("b"), tok::Operator(Op::Mul),
                   tok::Operator(Op::Tilde), tok::ID("c"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "+ - a * b ~ c");
  }

  // (a + b) * (c - d) / ~e
  {
    std::vector<Token> input = TokenArray(
        tok::ParenthesisL(), tok::ID("a"), tok::Operator(Op::Add), tok::ID("b"),
        tok::ParenthesisR(), tok::Operator(Op::Mul), tok::ParenthesisL(),
        tok::ID("c"), tok::Operator(Op::Sub), tok::ID("d"), tok::ParenthesisR(),
        tok::Operator(Op::Div), tok::Operator(Op::Tilde), tok::ID("e"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "/ * + a b - c d ~ e");
  }

  // a << b + c & d
  {
    std::vector<Token> input =
        TokenArray(tok::ID("a"), tok::Operator(Op::ShiftLeft), tok::ID("b"),
                   tok::Operator(Op::Add), tok::ID("c"),
                   tok::Operator(Op::BitAnd), tok::ID("d"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "& << a + b c d");
  }

  // ~a | b && c ^ d
  {
    std::vector<Token> input = TokenArray(
        tok::Operator(Op::Tilde), tok::ID("a"), tok::Operator(Op::BitOr),
        tok::ID("b"), tok::Operator(Op::LogicalAnd), tok::ID("c"),
        tok::Operator(Op::BitXor), tok::ID("d"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "&& | ~ a b ^ c d");
  }

  // a + b << c - ~d
  {
    std::vector<Token> input = TokenArray(
        tok::ID("a"), tok::Operator(Op::Add), tok::ID("b"),
        tok::Operator(Op::ShiftLeft), tok::ID("c"), tok::Operator(Op::Sub),
        tok::Operator(Op::Tilde), tok::ID("d"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "<< + a b - c ~ d");
  }

  // a && b | c ^ d & e
  {
    std::vector<Token> input = TokenArray(
        tok::ID("a"), tok::Operator(Op::LogicalAnd), tok::ID("b"),
        tok::Operator(Op::BitOr), tok::ID("c"), tok::Operator(Op::BitXor),
        tok::ID("d"), tok::Operator(Op::BitAnd), tok::ID("e"));
    EXPECT_EQ(ParseExpression(std::span(input))->Apply(get_prefix_visitor),
              "&& a | b ^ c & d e");
  }
}

TEST(ExprastParserTest, Skipper) {
  std::vector<Token> input =
      TokenArray(tok::WS(), tok::ID("a"), tok::Operator(Op::Add), tok::WS(),
                 tok::ID("b"), tok::WS(), tok::WS());
  EXPECT_EQ(ParseExpression(std::span(input))->DebugString(), "a+b");
}
