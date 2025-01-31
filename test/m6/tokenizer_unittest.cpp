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
#include "m6/tokenizer.hpp"

#include "util.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <variant>

using namespace m6;

using std::string_view_literals::operator""sv;
using std::string_literals::operator""s;

TEST(TokenizerTest, ID) {
  constexpr std::string_view input = "ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_, TokenArray(tok::ID(std::string(input))));
}

TEST(TokenizerTest, MultiID) {
  constexpr std::string_view input = "print ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::ID("print"s), tok::WS(), tok::ID("ObjFgInit"s)));
}

TEST(TokenizerTest, Numbers) {
  constexpr std::string_view input = "123 00321 -21";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::Int(123), tok::WS(), tok::Int(321), tok::WS(),
                       tok::Operator(Op::Sub), tok::Int(21)));
}

TEST(TokenizerTest, Brackets) {
  constexpr std::string_view input = "[]{}()";

  Tokenizer tokenizer(input);
  EXPECT_EQ(
      tokenizer.parsed_tok_,
      TokenArray(tok::SquareL(), tok::SquareR(), tok::CurlyL(), tok::CurlyR(),
                 tok::ParenthesisL(), tok::ParenthesisR()));
}

TEST(TokenizerTest, Operators) {
  constexpr std::string_view input =
      ", + - * / % & | ^ << >> ~ += -= *= /= %= &= |= ^= <<= >>= = == != <= < "
      ">= "
      "> && || ";
  Tokenizer tokenizer(input);

  std::vector<Token> result;
  std::copy_if(tokenizer.parsed_tok_.begin(), tokenizer.parsed_tok_.end(),
               std::back_inserter(result), [](const Token& x) {
                 return !std::holds_alternative<tok::WS>(x);
               });

  EXPECT_EQ(result,
            TokenArray(
                tok::Operator(Op::Comma), tok::Operator(Op::Add),
                tok::Operator(Op::Sub), tok::Operator(Op::Mul),
                tok::Operator(Op::Div), tok::Operator(Op::Mod),
                tok::Operator(Op::BitAnd), tok::Operator(Op::BitOr),
                tok::Operator(Op::BitXor), tok::Operator(Op::ShiftLeft),
                tok::Operator(Op::ShiftRight), tok::Operator(Op::Tilde),
                tok::Operator(Op::AddAssign), tok::Operator(Op::SubAssign),
                tok::Operator(Op::MulAssign), tok::Operator(Op::DivAssign),
                tok::Operator(Op::ModAssign), tok::Operator(Op::BitAndAssign),
                tok::Operator(Op::BitOrAssign), tok::Operator(Op::BitXorAssign),
                tok::Operator(Op::ShiftLeftAssign),
                tok::Operator(Op::ShiftRightAssign), tok::Operator(Op::Assign),
                tok::Operator(Op::Equal), tok::Operator(Op::NotEqual),
                tok::Operator(Op::LessEqual), tok::Operator(Op::Less),
                tok::Operator(Op::GreaterEqual), tok::Operator(Op::Greater),
                tok::Operator(Op::LogicalAnd), tok::Operator(Op::LogicalOr)));
}

TEST(TokenizerTest, StrLiteral) {
  {
    constexpr std::string_view input = R"("\"Hello\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(std::visit(tok::DebugStringVisitor(), result), R"(Str("Hello"))");
  }

  {
    constexpr std::string_view input = R"("\"He said, \\\"Hello\\\"\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(std::visit(tok::DebugStringVisitor(), result),
              R"(Str("He said, \"Hello\""))");
  }

  {
    constexpr std::string_view input = R"("")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(std::visit(tok::DebugStringVisitor(), result), "Str()");
  }

  {
    constexpr std::string_view input =
        R"("\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(std::visit(tok::DebugStringVisitor(), result),
              R"(Str("Path: C:\\Users\\Name\nNew Line\tTab"))");
  }
}
