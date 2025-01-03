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

#include "interpreter/tokenizer.hpp"

#include <string>
#include <string_view>
#include <variant>

using std::string_view_literals::operator""sv;
using std::string_literals::operator""s;

class TokenizerTest : public ::testing::Test {
 public:
  template <typename... Ts>
  std::vector<Token> TokenArray(Ts&&... args) {
    std::vector<Token> result;
    result.reserve(sizeof...(args));
    (result.emplace_back(std::forward<Ts>(args)), ...);
    return result;
  }
};

TEST_F(TokenizerTest, ParseID) {
  constexpr std::string_view input = "ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_, TokenArray(tok::ID(std::string(input))));
}

TEST_F(TokenizerTest, ParseMultiID) {
  constexpr std::string_view input = "print ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::ID("print"s), tok::WS(), tok::ID("ObjFgInit"s)));
}

TEST_F(TokenizerTest, ParseNumbers) {
  constexpr std::string_view input = "123 00321 -21";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::Int(123), tok::WS(), tok::Int(321), tok::WS(),
                       tok::Minus(), tok::Int(21)));
}

TEST_F(TokenizerTest, ParseSymbols) {
  constexpr std::string_view input = "$a_=0+1-2/3*5";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::Dollar(), tok::ID("a_"s), tok::Eq(), tok::Int(0),
                       tok::Plus(), tok::Int(1), tok::Minus(), tok::Int(2),
                       tok::Div(), tok::Int(3), tok::Mult(), tok::Int(5)));
}

TEST_F(TokenizerTest, ParseBrackets) {
  constexpr std::string_view input = "[]{}()";

  Tokenizer tokenizer(input);
  EXPECT_EQ(
      tokenizer.parsed_tok_,
      TokenArray(tok::SquareL(), tok::SquareR(), tok::CurlyL(), tok::CurlyR(),
                 tok::ParenthesisL(), tok::ParenthesisR()));
}
