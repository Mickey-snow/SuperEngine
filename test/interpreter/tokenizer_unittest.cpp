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

#include "util.hpp"

#include <string>
#include <string_view>
#include <variant>

using std::string_view_literals::operator""sv;
using std::string_literals::operator""s;

TEST(TokenizerTest, ParseID) {
  constexpr std::string_view input = "ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_, TokenArray(tok::ID(std::string(input))));
}

TEST(TokenizerTest, ParseMultiID) {
  constexpr std::string_view input = "print ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::ID("print"s), tok::WS(), tok::ID("ObjFgInit"s)));
}

TEST(TokenizerTest, ParseNumbers) {
  constexpr std::string_view input = "123 00321 -21";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::Int(123), tok::WS(), tok::Int(321), tok::WS(),
                       tok::Minus(), tok::Int(21)));
}

TEST(TokenizerTest, ParseSymbols) {
  constexpr std::string_view input = "$a_=0+1-2/3*5";

  Tokenizer tokenizer(input);
  EXPECT_EQ(tokenizer.parsed_tok_,
            TokenArray(tok::Dollar(), tok::ID("a_"s), tok::Eq(), tok::Int(0),
                       tok::Plus(), tok::Int(1), tok::Minus(), tok::Int(2),
                       tok::Div(), tok::Int(3), tok::Mult(), tok::Int(5)));
}

TEST(TokenizerTest, ParseBrackets) {
  constexpr std::string_view input = "[]{}()";

  Tokenizer tokenizer(input);
  EXPECT_EQ(
      tokenizer.parsed_tok_,
      TokenArray(tok::SquareL(), tok::SquareR(), tok::CurlyL(), tok::CurlyR(),
                 tok::ParenthesisL(), tok::ParenthesisR()));
}

TEST(TokenizerHelperTest, MatchTok) {
  Token tok = tok::Dollar();
  EXPECT_TRUE(match_token<tok::Dollar>()(tok));
  EXPECT_FALSE(match_token<tok::ID>()(tok));
}

TEST(TokenizerHelperTest, MatchInt) {
  Token tok = tok::Int(12);
  int value = 0;
  EXPECT_FALSE(match_int()(tok::Dollar(), value));
  EXPECT_EQ(value, 0);
  EXPECT_TRUE(match_int()(tok, value));
  EXPECT_EQ(value, 12);
}

TEST(TokenizerHelperTest, MatchID) {
  Token tok = tok::ID("intL");
  std::string value;
  EXPECT_FALSE(match_id()(tok::Dollar(), value));
  EXPECT_TRUE(value.empty());
  EXPECT_TRUE(match_id()(tok, value));
  EXPECT_EQ(value, "intL");
}
