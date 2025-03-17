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

#include "m6/tokenizer.hpp"
#include "machine/op.hpp"

#include "util.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <string_view>
#include <variant>

namespace m6test {

using namespace m6;
using std::string_view_literals::operator""sv;
using std::string_literals::operator""s;

inline std::string Accumulate(const auto& container) {
  return std::accumulate(container.cbegin(), container.cend(), std::string(),
                         [](std::string&& s, Token const& t) {
                           return (s.empty() ? s : (s + ' ')) +
                                  t.GetDebugString();
                         });
}

TEST(TokenizerTest, ID) {
  constexpr std::string_view input = "ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<ID(\"ObjFgInit\"), 0> <EOF, 9>");
}

TEST(TokenizerTest, MultiID) {
  constexpr std::string_view input = "print ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<ID(\"print\"), 0> <ws, 5> <ID(\"ObjFgInit\"), 6> <EOF, 15>");
}

TEST(TokenizerTest, Numbers) {
  constexpr std::string_view input = "123 00321 -21";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<Int(123), 0> <ws, 3> <Int(321), 4> <ws, 9> <Operator(-), 10> "
            "<Int(21), 11> <EOF, 13>");
}

TEST(TokenizerTest, Brackets) {
  constexpr std::string_view input = "[]{}()";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<SquareL, 0> <SquareR, 1> <CurlyL, 2> <CurlyR, 3> <ParenthesisL, "
            "4> <ParenthesisR, 5> <EOF, 6>");
}

TEST(TokenizerTest, Operators) {
  constexpr std::string_view input =
      ". , + - * / % & | ^ << >> ~ += -= *= /= %= &= |= ^= <<= >>= = == != <= "
      "< >= > && || >>> >>>=";
  Tokenizer tokenizer(input);

  EXPECT_EQ(
      Accumulate(tokenizer.parsed_tok_),
      "<Operator(.), 0> <ws, 1> <Operator(,), 2> <ws, 3> <Operator(+), 4> <ws, "
      "5> <Operator(-), 6> <ws, 7> <Operator(*), 8> <ws, 9> <Operator(/), 10> "
      "<ws, 11> <Operator(%), 12> <ws, 13> <Operator(&), 14> <ws, 15> "
      "<Operator(|), 16> <ws, 17> <Operator(^), 18> <ws, 19> <Operator(<<), "
      "20> <ws, 22> <Operator(>>), 23> <ws, 25> <Operator(~), 26> <ws, 27> "
      "<Operator(+=), 28> <ws, 30> <Operator(-=), 31> <ws, 33> <Operator(*=), "
      "34> <ws, 36> <Operator(/=), 37> <ws, 39> <Operator(%=), 40> <ws, 42> "
      "<Operator(&=), 43> <ws, 45> <Operator(|=), 46> <ws, 48> <Operator(^=), "
      "49> <ws, 51> <Operator(<<=), 52> <ws, 55> <Operator(>>=), 56> <ws, 59> "
      "<Operator(=), 60> <ws, 61> <Operator(==), 62> <ws, 64> <Operator(!=), "
      "65> <ws, 67> <Operator(<=), 68> <ws, 70> <Operator(<), 71> <ws, 72> "
      "<Operator(>=), 73> <ws, 75> <Operator(>), 76> <ws, 77> <Operator(&&), "
      "78> <ws, 80> <Operator(||), 81> <ws, 83> <Operator(>>>), 84> <ws, 87> "
      "<Operator(>>>=), 88> <EOF, 92>");
}

TEST(TokenizerTest, StrLiteral) {
  {
    constexpr std::string_view input = R"("\"Hello\"")";
    Tokenizer tokenizer(input);

    EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
              "<Str(\"Hello\"), 0> <EOF, 11>");
  }

  {
    constexpr std::string_view input = R"("\"He said, \\\"Hello\\\"\"")";
    Tokenizer tokenizer(input);

    EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
              "<Str(\"He said, \\\"Hello\\\"\"), 0> <EOF, 28>");
  }

  {
    constexpr std::string_view input = R"("")";
    Tokenizer tokenizer(input);

    EXPECT_EQ(Accumulate(tokenizer.parsed_tok_), "<Str(), 0> <EOF, 2>");
  }

  {
    constexpr std::string_view input =
        R"("\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"")";
    Tokenizer tokenizer(input);

    EXPECT_EQ(
        Accumulate(tokenizer.parsed_tok_),
        "<Str(\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"), 0> <EOF, 48>");
  }
}

TEST(TokenizerTest, UnclosedString) {
  constexpr std::string_view input = "\"hello";
  Tokenizer tokenizer(input);

  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<Str(\"hello), 0> <Error(Expected '\"'), 6> <EOF, 6>");
}

TEST(TokenizerTest, UnknownToken) {
  constexpr std::string_view input = "id\xff\xff+32";
  Tokenizer tokenizer(input);

  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "<ID(\"id\"), 0> <Error(Unknown token), 2> <Operator(+), 4> "
            "<Int(32), 5> <EOF, 7>");
}

}  // namespace m6test
