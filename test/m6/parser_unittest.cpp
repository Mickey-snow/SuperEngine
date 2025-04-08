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

#include "m6/ast.hpp"
#include "m6/parser.hpp"
#include "m6/script_engine.hpp"
#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"

#include <string_view>

using std::string_view_literals::operator""sv;
using namespace m6;

namespace m6test {

static inline std::string GetErrors(std::string input) {
  ScriptEngine engine;
  auto result = engine.Execute(std::move(input));

  return Join("\n", std::views::all(result.errors) |
                        std::views::transform([](const auto& e) {
                          std::string loc = "(?)";
                          if (e.loc.has_value())
                            loc = static_cast<std::string>(*e.loc);
                          return e.msg + ' ' + loc;
                        }));
  // if (result.errors.empty())
  //   return "it throws nothing.";
  // else
  //   return engine.FlushErrors();
}

class ExprParserTest : public ::testing::Test {
 protected:
  static std::shared_ptr<ExprAST> parseExpr(std::span<Token> tokens) {
    std::shared_ptr<ExprAST> result = nullptr;
    EXPECT_NO_THROW(result = ParseExpression(tokens));
    EXPECT_NE(result, nullptr);
    return result;
  }

  static void expectAST(std::vector<Token> tokens, const char* expectedAST) {
    auto result = parseExpr(tokens);
    EXPECT_TXTEQ(result->DumpAST(), expectedAST);
  }
};

TEST_F(ExprParserTest, BasicArithmetic) {
  expectAST(TokenArray(tok::Int(1), tok::Operator(Op::Add), tok::Int(2)),
            R"(
Binaryop +
   ├─IntLiteral 1
   └─IntLiteral 2
)");

  expectAST(TokenArray(tok::Int(3), tok::Operator(Op::Sub), tok::Int(4)),
            R"(
Binaryop -
   ├─IntLiteral 3
   └─IntLiteral 4
)");

  expectAST(TokenArray(tok::Int(5), tok::Operator(Op::Mul), tok::Int(6)),
            R"(
Binaryop *
   ├─IntLiteral 5
   └─IntLiteral 6
)");

  expectAST(TokenArray(tok::Int(7), tok::Operator(Op::Div), tok::Int(8)),
            R"(
Binaryop /
   ├─IntLiteral 7
   └─IntLiteral 8
)");

  expectAST(TokenArray(tok::Int(9), tok::Operator(Op::Mod), tok::Int(10)),
            R"(
Binaryop %
   ├─IntLiteral 9
   └─IntLiteral 10
)");
}

TEST_F(ExprParserTest, Precedence) {
  expectAST(TokenArray(tok::Int(5), tok::Operator(Op::Mul), tok::Int(6),
                       tok::Operator(Op::Add), tok::Int(7)),
            R"(
Binaryop +
   ├─Binaryop *
   │  ├─IntLiteral 5
   │  └─IntLiteral 6
   └─IntLiteral 7
)");

  expectAST(TokenArray(tok::Int(5), tok::Operator(Op::Add), tok::Int(6),
                       tok::Operator(Op::Div), tok::Int(7)),
            R"(
Binaryop +
   ├─IntLiteral 5
   └─Binaryop /
      ├─IntLiteral 6
      └─IntLiteral 7
)");
}

TEST_F(ExprParserTest, Parenthesis) {
  expectAST(TokenArray(tok::ParenthesisL(), tok::Int(5), tok::Operator(Op::Add),
                       tok::Int(6), tok::ParenthesisR(), tok::Operator(Op::Div),
                       tok::Int(7)),
            R"(
Binaryop /
   ├─Parenthesis
   │  └─Binaryop +
   │     ├─IntLiteral 5
   │     └─IntLiteral 6
   └─IntLiteral 7
)");
}

TEST_F(ExprParserTest, Identifier) {
  expectAST(TokenArray(tok::ID("v1"), tok::Operator(Op::Add), tok::ID("v2"),
                       tok::Operator(Op::Div), tok::ID("v3"), tok::SquareL(),
                       tok::ID("v4"), tok::Operator(Op::Add), tok::ID("v5"),
                       tok::SquareR()),
            R"(
Binaryop +
   ├─ID v1
   └─Binaryop /
      ├─ID v2
      └─Subscript
         ├─ID v3
         └─Binaryop +
            ├─ID v4
            └─ID v5
)");
}

TEST_F(ExprParserTest, Comparisons) {
  expectAST(TokenArray(tok::ID("v1"), tok::Operator(Op::Equal), tok::ID("v2"),
                       tok::Operator(Op::NotEqual), tok::ID("v3"),
                       tok::Operator(Op::Greater), tok::ID("v4"),
                       tok::Operator(Op::Less), tok::ID("v5"),
                       tok::Operator(Op::LessEqual), tok::Int(12),
                       tok::Operator(Op::GreaterEqual), tok::Int(13)),
            R"(
Binaryop !=
   ├─Binaryop ==
   │  ├─ID v1
   │  └─ID v2
   └─Binaryop >=
      ├─Binaryop <=
      │  ├─Binaryop <
      │  │  ├─Binaryop >
      │  │  │  ├─ID v3
      │  │  │  └─ID v4
      │  │  └─ID v5
      │  └─IntLiteral 12
      └─IntLiteral 13
)");
}

TEST_F(ExprParserTest, Shifts) {
  expectAST(TokenArray(tok::ID("v1"), tok::Operator(Op::ShiftLeft),
                       tok::ID("v2"), tok::Operator(Op::Less), tok::ID("v3"),
                       tok::Operator(Op::ShiftRight), tok::ID("v4"),
                       tok::Operator(Op::Add), tok::ID("v5"),
                       tok::Operator(Op::ShiftLeft), tok::Int(12),
                       tok::Operator(Op::Less), tok::Int(13)),
            R"(
Binaryop <
   ├─Binaryop <
   │  ├─Binaryop <<
   │  │  ├─ID v1
   │  │  └─ID v2
   │  └─Binaryop <<
   │     ├─Binaryop >>
   │     │  ├─ID v3
   │     │  └─Binaryop +
   │     │     ├─ID v4
   │     │     └─ID v5
   │     └─IntLiteral 12
   └─IntLiteral 13
)");
}

TEST_F(ExprParserTest, Logical) {
  expectAST(
      TokenArray(tok::ID("v1"), tok::Operator(Op::LogicalOr), tok::ID("v2"),
                 tok::Operator(Op::LogicalAnd), tok::ID("v3"),
                 tok::Operator(Op::ShiftRight), tok::ID("v4"),
                 tok::Operator(Op::LogicalOr), tok::ID("v5"),
                 tok::Operator(Op::LogicalAnd), tok::Int(12)),
      R"(
Binaryop ||
   ├─Binaryop ||
   │  ├─ID v1
   │  └─Binaryop &&
   │     ├─ID v2
   │     └─Binaryop >>
   │        ├─ID v3
   │        └─ID v4
   └─Binaryop &&
      ├─ID v5
      └─IntLiteral 12
)");
}

TEST_F(ExprParserTest, BitwiseOperators) {
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b")),
            R"(
Binaryop &
   ├─ID a
   └─ID b
)");

  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::BitOr), tok::ID("b")),
            R"(
Binaryop |
   ├─ID a
   └─ID b
)");

  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::BitXor), tok::ID("b")),
            R"(
Binaryop ^
   ├─ID a
   └─ID b
)");

  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b"),
                       tok::Operator(Op::BitOr), tok::ID("c"),
                       tok::Operator(Op::BitXor), tok::ID("d")),
            R"(
Binaryop |
   ├─Binaryop &
   │  ├─ID a
   │  └─ID b
   └─Binaryop ^
      ├─ID c
      └─ID d
)");
}

TEST_F(ExprParserTest, UnaryOperators) {
  expectAST(TokenArray(tok::Operator(Op::Sub), tok::ID("a")),
            R"(
Unaryop -
   └─ID a
)");

  expectAST(TokenArray(tok::Operator(Op::Add), tok::ID("a")),
            R"(
Unaryop +
   └─ID a
)");

  expectAST(TokenArray(tok::Operator(Op::Tilde), tok::ID("a")),
            R"(
Unaryop ~
   └─ID a
)");

  expectAST(TokenArray(tok::Operator(Op::Sub), tok::Operator(Op::Tilde),
                       tok::ID("a")),
            R"(
Unaryop -
   └─Unaryop ~
      └─ID a
)");

  expectAST(
      TokenArray(tok::Operator(Op::Sub), tok::ParenthesisL(), tok::ID("a"),
                 tok::Operator(Op::Add), tok::ID("b"), tok::ParenthesisR()),
      R"(
Unaryop -
   └─Parenthesis
      └─Binaryop +
         ├─ID a
         └─ID b
)");
}

TEST_F(ExprParserTest, MixedPrecedence) {
  // a + b * c
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::Add), tok::ID("b"),
                       tok::Operator(Op::Mul), tok::ID("c")),
            R"(
Binaryop +
   ├─ID a
   └─Binaryop *
      ├─ID b
      └─ID c
)");

  // a & b | c ^ d
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::BitAnd), tok::ID("b"),
                       tok::Operator(Op::BitOr), tok::ID("c"),
                       tok::Operator(Op::BitXor), tok::ID("d")),
            R"(
Binaryop |
   ├─Binaryop &
   │  ├─ID a
   │  └─ID b
   └─Binaryop ^
      ├─ID c
      └─ID d
)");

  // -a + b * ~c
  expectAST(
      TokenArray(tok::Operator(Op::Sub), tok::ID("a"), tok::Operator(Op::Add),
                 tok::ID("b"), tok::Operator(Op::Mul), tok::Operator(Op::Tilde),
                 tok::ID("c")),
      R"(
Binaryop +
   ├─Unaryop -
   │  └─ID a
   └─Binaryop *
      ├─ID b
      └─Unaryop ~
         └─ID c
)");

  // (a + b) * (c - d) / ~e
  expectAST(
      TokenArray(tok::ParenthesisL(), tok::ID("a"), tok::Operator(Op::Add),
                 tok::ID("b"), tok::ParenthesisR(), tok::Operator(Op::Mul),
                 tok::ParenthesisL(), tok::ID("c"), tok::Operator(Op::Sub),
                 tok::ID("d"), tok::ParenthesisR(), tok::Operator(Op::Div),
                 tok::Operator(Op::Tilde), tok::ID("e")),
      R"(
Binaryop /
   ├─Binaryop *
   │  ├─Parenthesis
   │  │  └─Binaryop +
   │  │     ├─ID a
   │  │     └─ID b
   │  └─Parenthesis
   │     └─Binaryop -
   │        ├─ID c
   │        └─ID d
   └─Unaryop ~
      └─ID e
)");

  // a << b + c & d
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::ShiftLeft), tok::ID("b"),
                       tok::Operator(Op::Add), tok::ID("c"),
                       tok::Operator(Op::BitAnd), tok::ID("d")),
            R"(
Binaryop &
   ├─Binaryop <<
   │  ├─ID a
   │  └─Binaryop +
   │     ├─ID b
   │     └─ID c
   └─ID d
)");

  // ~a | b && c ^ d
  expectAST(TokenArray(tok::Operator(Op::Tilde), tok::ID("a"),
                       tok::Operator(Op::BitOr), tok::ID("b"),
                       tok::Operator(Op::LogicalAnd), tok::ID("c"),
                       tok::Operator(Op::BitXor), tok::ID("d")),
            R"(
Binaryop &&
   ├─Binaryop |
   │  ├─Unaryop ~
   │  │  └─ID a
   │  └─ID b
   └─Binaryop ^
      ├─ID c
      └─ID d
)");

  // a + b << c - ~d
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::Add), tok::ID("b"),
                       tok::Operator(Op::ShiftLeft), tok::ID("c"),
                       tok::Operator(Op::Sub), tok::Operator(Op::Tilde),
                       tok::ID("d")),
            R"(
Binaryop <<
   ├─Binaryop +
   │  ├─ID a
   │  └─ID b
   └─Binaryop -
      ├─ID c
      └─Unaryop ~
         └─ID d
)");

  // a && b | c ^ d & e
  expectAST(TokenArray(tok::ID("a"), tok::Operator(Op::LogicalAnd),
                       tok::ID("b"), tok::Operator(Op::BitOr), tok::ID("c"),
                       tok::Operator(Op::BitXor), tok::ID("d"),
                       tok::Operator(Op::BitAnd), tok::ID("e")),
            R"(
Binaryop &&
   ├─ID a
   └─Binaryop |
      ├─ID b
      └─Binaryop ^
         ├─ID c
         └─Binaryop &
            ├─ID d
            └─ID e
)");

  // a >>>= b >>> c;
  {
    // This one uses ParseStmt, so keep as a minimal inline check of ThrowResult
    // or parseStmt.
    std::vector<Token> input = TokenArray(
        tok::ID("a"), tok::Operator(Op::ShiftUnsignedRightAssign), tok::ID("b"),
        tok::Operator(Op::ShiftUnsignedRight), tok::ID("c"), tok::Semicol());
    std::shared_ptr<AST> result = nullptr;
    ASSERT_NO_THROW(result = ParseStmt(std::span(input)));
    ASSERT_NE(result, nullptr);
    EXPECT_TXTEQ(result->DumpAST(), R"(
AugAssign >>>=
   ├─ID a
   └─Binaryop >>>
      ├─ID b
      └─ID c
)");
  }
}

TEST_F(ExprParserTest, StringLiterals) {
  expectAST(
      TokenArray(tok::ID("foo"), tok::Operator(Op::Add), tok::Literal("bar")),
      R"(
Binaryop +
   ├─ID foo
   └─StrLiteral bar
)");
}

TEST_F(ExprParserTest, Postfix) {
  expectAST(TokenArray(tok::ID("foo"), tok::ParenthesisL(), tok::Int(42),
                       tok::ParenthesisR()),
            R"(
Invoke
   ├─ID foo
   └─IntLiteral 42
)");

  expectAST(
      TokenArray(tok::ID("sum"), tok::ParenthesisL(), tok::Int(1),
                 tok::Operator(Op::Comma), tok::Int(2),
                 tok::Operator(Op::Comma), tok::Int(3),
                 tok::Operator(Op::Comma), tok::Int(4), tok::ParenthesisR()),
      R"(
Invoke
   ├─ID sum
   ├─IntLiteral 1
   ├─IntLiteral 2
   ├─IntLiteral 3
   └─IntLiteral 4
)");

  expectAST(
      TokenArray(tok::ID("array"), tok::SquareL(), tok::Int(3), tok::SquareR(),
                 tok::Operator(Op::Dot), tok::ID("field")),
      R"(
Member
   ├─Subscript
   │  ├─ID array
   │  └─IntLiteral 3
   └─ID field
)");

  expectAST(
      TokenArray(tok::ID("obj"), tok::Operator(Op::Dot), tok::ID("getArray"),
                 tok::ParenthesisL(), tok::ParenthesisR(), tok::SquareL(),
                 tok::ID("idx"), tok::Operator(Op::Add), tok::Int(1),
                 tok::SquareR(), tok::Operator(Op::Dot), tok::ID("method"),
                 tok::ParenthesisL(), tok::Int(10), tok::ParenthesisR()),
      R"(
Invoke
   ├─Member
   │  ├─Subscript
   │  │  ├─Invoke
   │  │  │  ├─Member
   │  │  │  │  ├─ID obj
   │  │  │  │  └─ID getArray
   │  │  └─Binaryop +
   │  │     ├─ID idx
   │  │     └─IntLiteral 1
   │  └─ID method
   └─IntLiteral 10
)");

  expectAST(
      TokenArray(tok::ParenthesisL(), tok::ID("foo"), tok::ParenthesisL(),
                 tok::ID("bar"), tok::ParenthesisL(), tok::Int(1),
                 tok::ParenthesisR(), tok::ParenthesisR(),
                 tok::Operator(Op::Dot), tok::ID("baz"), tok::ParenthesisR(),
                 tok::SquareL(), tok::Literal("3"), tok::SquareR()),
      R"(
Subscript
   ├─Parenthesis
   │  └─Member
   │     ├─Invoke
   │     │  ├─ID foo
   │     │  └─Invoke
   │     │     ├─ID bar
   │     │     └─IntLiteral 1
   │     └─ID baz
   └─StrLiteral 3
)");
}

TEST_F(ExprParserTest, ErrorHandling) {
  EXPECT_EQ(GetErrors("a+(b*c"),
            "Missing closing ')' in parenthesized expression. (6,6)");

  EXPECT_EQ(GetErrors("d / / e"), "Expected primary expression. (4,5)");

  EXPECT_EQ(GetErrors("f* (g+)"), "Expected primary expression. (6,7)");

  EXPECT_EQ(GetErrors("j+*k"), "Expected primary expression. (2,3)");

  EXPECT_EQ(GetErrors(")a"), "Expected primary expression. (0,1)");

  EXPECT_EQ(GetErrors("a."), "Expected identifier after '.' (2,3)");

  EXPECT_EQ(GetErrors("a[123"),
            "Expected ']' after subscript expression. (5,6)");

  EXPECT_EQ(GetErrors("a(b,c"), "Expected ')' after function call. (5,5)");

  EXPECT_EQ(GetErrors("a=1+1"), "Expected ';'. (5,5)");
}

// -----------------------------------------------------------------------

class StmtParserTest : public ::testing::Test {
 protected:
  static std::shared_ptr<AST> parseStmt(std::span<Token> tokens) {
    std::shared_ptr<AST> result = nullptr;
    EXPECT_NO_THROW(result = ParseStmt(std::span(tokens)));
    EXPECT_NE(result, nullptr);
    return result;
  }

  static void expectStmtAST(std::vector<Token> tokens,
                            const char* expectedAST) {
    auto result = parseStmt(tokens);
    EXPECT_TXTEQ(result->DumpAST(), expectedAST);
  }
};

TEST_F(StmtParserTest, Assignment) {
  {  // Basic variable assignment
    expectStmtAST(TokenArray("v1 = 1 + 2 - 3;"sv),
                  R"(
Assign
   ├─ID v1
   └─Binaryop -
      ├─Binaryop +
      │  ├─IntLiteral 1
      │  └─IntLiteral 2
      └─IntLiteral 3
)");
  }

  {  // Basic compound assignment
    expectStmtAST(TokenArray("v1+=x+y-y%x;"sv),
                  R"(
AugAssign +=
   ├─ID v1
   └─Binaryop -
      ├─Binaryop +
      │  ├─ID x
      │  └─ID y
      └─Binaryop %
         ├─ID y
         └─ID x
)");
  }

  {  // error: assigning to expression
    EXPECT_TXTEQ(GetErrors("(v1+v2) = 1"), "Expected identifier. (0,7)");
  }
}

TEST_F(StmtParserTest, If) {
  expectStmtAST(TokenArray("if(a) b; else c;"sv),
                R"(
If
   ├─cond
   │  └─ID a
   ├─then
   │  └─ID b
   └─else
      └─ID c
)");
}

TEST_F(StmtParserTest, While) {
  expectStmtAST(TokenArray("while(i<10) i+=1;"sv),
                R"(
While
   ├─cond
   │  └─Binaryop <
   │     ├─ID i
   │     └─IntLiteral 10
   └─body
      └─AugAssign +=
         ├─ID i
         └─IntLiteral 1
)");
}

TEST_F(StmtParserTest, For) {
  expectStmtAST(TokenArray("for(i=0;i<10;i+=1) sum += i;"sv),
                R"(
For
   ├─init
   │  └─Assign
   │     ├─ID i
   │     └─IntLiteral 0
   ├─cond
   │  └─Binaryop <
   │     ├─ID i
   │     └─IntLiteral 10
   ├─inc
   │  └─AugAssign +=
   │     ├─ID i
   │     └─IntLiteral 1
   └─body
      └─AugAssign +=
         ├─ID sum
         └─ID i
)");
}

TEST_F(StmtParserTest, Block) {
  expectStmtAST(TokenArray("{i=1;j=2;k=3;l=4; {}}"sv), R"(
Compound
   ├─Assign
   │  ├─ID i
   │  └─IntLiteral 1
   ├─Assign
   │  ├─ID j
   │  └─IntLiteral 2
   ├─Assign
   │  ├─ID k
   │  └─IntLiteral 3
   ├─Assign
   │  ├─ID l
   │  └─IntLiteral 4
   └─Compound
)");
}

}  // namespace m6test
