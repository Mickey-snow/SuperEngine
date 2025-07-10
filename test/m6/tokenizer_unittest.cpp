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

#include "m6/source_buffer.hpp"
#include "m6/tokenizer.hpp"
#include "machine/op.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <string_view>
#include <variant>

namespace m6test {

using namespace m6;

inline std::string Accumulate(const auto& container) {
  return std::accumulate(container.cbegin(), container.cend(), std::string(),
                         [](std::string&& s, Token const& t) {
                           return (s.empty() ? s : (s + ' ')) +
                                  t.GetDebugString();
                         });
}

class TokenizerTest : public ::testing::Test {
 protected:
  TokenizerTest() : tokens(), tokenizer(tokens) {
    tokenizer.add_eof_ = false;
    tokenizer.skip_ws_ = true;
  }

  std::vector<Token> tokens;
  Tokenizer tokenizer;
};

TEST_F(TokenizerTest, ID) {
  const std::string input = "ObjFgInit";

  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));
  EXPECT_EQ(Accumulate(tokens), "<ID(\"ObjFgInit\"), 0,9>");
}

TEST_F(TokenizerTest, MultiID) {
  const std::string input = "print ObjFgInit";

  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));
  EXPECT_EQ(Accumulate(tokens),
            "<ID(\"print\"), 0,5> <ID(\"ObjFgInit\"), 6,15>");
}

TEST_F(TokenizerTest, Numbers) {
  const std::string input = "123 00321 -21";

  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));
  EXPECT_EQ(
      Accumulate(tokens),
      "<Int(123), 0,3> <Int(321), 4,9> <Operator(-), 10,11> <Int(21), 11,13>");
}

TEST_F(TokenizerTest, Brackets) {
  const std::string input = "[]{}()";

  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));
  EXPECT_EQ(Accumulate(tokens),
            "<SquareL, 0,1> <SquareR, 1,2> <CurlyL, 2,3> <CurlyR, 3,4> "
            "<ParenthesisL, 4,5> <ParenthesisR, 5,6>");
}

TEST_F(TokenizerTest, Operators) {
  const std::string input =
      ". , + - * / % & | ^ << >> ~ += -= *= /= %= &= |= ^= <<= >>= = == != "
      "<= "
      "< >= > && || >>> >>>= ** **=";
  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

  EXPECT_EQ(
      Accumulate(tokens),
      "<Operator(.), 0,1> <Operator(,), 2,3> <Operator(+), 4,5> <Operator(-), "
      "6,7> <Operator(*), 8,9> <Operator(/), 10,11> <Operator(%), 12,13> "
      "<Operator(&), 14,15> <Operator(|), 16,17> <Operator(^), 18,19> "
      "<Operator(<<), 20,22> <Operator(>>), 23,25> <Operator(~), 26,27> "
      "<Operator(+=), 28,30> <Operator(-=), 31,33> <Operator(*=), 34,36> "
      "<Operator(/=), 37,39> <Operator(%=), 40,42> <Operator(&=), 43,45> "
      "<Operator(|=), 46,48> <Operator(^=), 49,51> <Operator(<<=), 52,55> "
      "<Operator(>>=), 56,59> <Operator(=), 60,61> <Operator(==), 62,64> "
      "<Operator(!=), 65,67> <Operator(<=), 68,70> <Operator(<), 71,72> "
      "<Operator(>=), 73,75> <Operator(>), 76,77> <Operator(&&), 78,80> "
      "<Operator(||), 81,83> <Operator(>>>), 84,87> <Operator(>>>=), 88,92> "
      "<Operator(**), 93,95> <Operator(**=), 96,99>");
}

TEST_F(TokenizerTest, StrLiteral) {
  {
    const std::string input = R"("\"Hello\"")";
    tokens.clear();
    tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

    EXPECT_EQ(Accumulate(tokens), "<Str(\"Hello\"), 0,11>");
  }

  {
    const std::string input = R"("\"He said, \\\"Hello\\\"\"")";
    tokens.clear();
    tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

    EXPECT_EQ(Accumulate(tokens), "<Str(\"He said, \\\"Hello\\\"\"), 0,28>");
  }

  {
    const std::string input = R"("")";
    tokens.clear();
    tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

    EXPECT_EQ(Accumulate(tokens), "<Str(), 0,2>");
  }

  {
    const std::string input =
        R"("\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"")";
    tokens.clear();
    tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

    EXPECT_EQ(Accumulate(tokens),
              "<Str(\"Path: C:\\\\Users\\\\Name\\nNew Line\\tTab\"), 0,48>");
  }
}

TEST_F(TokenizerTest, Yield) {
  const std::string input = "yield";
  tokens.clear();
  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

  EXPECT_EQ(Accumulate(tokens), "<Reserved(yield), 0,5>");
}

TEST_F(TokenizerTest, UnclosedString) {
  const std::string input = "\"hello";
  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

  EXPECT_EQ(Accumulate(tokens), "<Str(\"hello), 0,6>");
}

TEST_F(TokenizerTest, UnknownToken) {
  const std::string input = "id\xff\xff+32";
  tokenizer.Parse(SourceBuffer::Create(input, "<test>"));

  EXPECT_EQ(Accumulate(tokens),
            "<ID(\"id\"), 0,2> <Operator(+), 4,5> <Int(32), 5,7>");
}

}  // namespace m6test
