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
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_), "ID(\"ObjFgInit\")");
}

TEST(TokenizerTest, MultiID) {
  constexpr std::string_view input = "print ObjFgInit";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "ID(\"print\") <ws> ID(\"ObjFgInit\")");
}

TEST(TokenizerTest, Numbers) {
  constexpr std::string_view input = "123 00321 -21";

  Tokenizer tokenizer(input);
  EXPECT_EQ(Accumulate(tokenizer.parsed_tok_),
            "Int(123) <ws> Int(321) <ws> Operator(-) Int(21)");
}

TEST(TokenizerTest, Brackets) {
  constexpr std::string_view input = "[]{}()";

  Tokenizer tokenizer(input);
  EXPECT_EQ(
      Accumulate(tokenizer.parsed_tok_),
      "<SquareL> <SquareR> <CurlyL> <CurlyR> <ParenthesisL> <ParenthesisR>");
}

TEST(TokenizerTest, Operators) {
  constexpr std::string_view input =
      ". , + - * / % & | ^ << >> ~ += -= *= /= %= &= |= ^= <<= >>= = == != <= "
      "< >= > && || >>> >>>=";
  Tokenizer tokenizer(input);

  EXPECT_EQ(
      Accumulate(tokenizer.parsed_tok_),
      "Operator(.) <ws> Operator(,) <ws> Operator(+) <ws> Operator(-) <ws> "
      "Operator(*) <ws> Operator(/) <ws> Operator(%) <ws> Operator(&) <ws> "
      "Operator(|) <ws> Operator(^) <ws> Operator(<<) <ws> Operator(>>) <ws> "
      "Operator(~) <ws> Operator(+=) <ws> Operator(-=) <ws> Operator(*=) <ws> "
      "Operator(/=) <ws> Operator(%=) <ws> Operator(&=) <ws> Operator(|=) <ws> "
      "Operator(^=) <ws> Operator(<<=) <ws> Operator(>>=) <ws> Operator(=) "
      "<ws> Operator(==) <ws> Operator(!=) <ws> Operator(<=) <ws> Operator(<) "
      "<ws> Operator(>=) <ws> Operator(>) <ws> Operator(&&) <ws> Operator(||) "
      "<ws> Operator(>>>) <ws> Operator(>>>=)");
}

TEST(TokenizerTest, StrLiteral) {
  {
    constexpr std::string_view input = R"("\"Hello\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(result.GetDebugString(), R"(Str("Hello"))");
  }

  {
    constexpr std::string_view input = R"("\"He said, \\\"Hello\\\"\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(result.GetDebugString(), R"(Str("He said, \"Hello\""))");
  }

  {
    constexpr std::string_view input = R"("")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(result.GetDebugString(), "Str()");
  }

  {
    constexpr std::string_view input =
        R"("\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"")";
    Tokenizer tokenizer(input);

    auto result = tokenizer.parsed_tok_.front();
    EXPECT_EQ(result.GetDebugString(),
              R"(Str("Path: C:\\Users\\Name\nNew Line\tTab"))");
  }
}

}  // namespace m6test
