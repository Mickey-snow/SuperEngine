// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "libsiglus/element.hpp"
#include "libsiglus/lexer.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

using namespace libsiglus;

template <typename T>
inline static std::string_view vec_to_sv(const std::vector<T>& vec) {
  return std::string_view(reinterpret_cast<const char*>(vec.data()),
                          vec.size() * sizeof(T));
}

class LexerTest : public ::testing::Test {
 protected:
  Lexer lex;
};

TEST_F(LexerTest, Newline) {
  std::vector<uint8_t> raw_nl{
      0x01,                    // newline
      0x0a, 0x00, 0x00, 0x00,  // 10
      0xff, 0xab               // garbage
  };

  auto result = lex.Parse(vec_to_sv(raw_nl));
  EXPECT_EQ(result->ToDebugString(), "#line 10");
}

TEST_F(LexerTest, Pushstk) {
  {
    std::vector<uint8_t> raw{
        0x02,  // push
        0x0a, 0x00, 0x00,
        0x00,  // int
        0x3f, 0x00, 0x00,
        0x00,  // 63
        0xaa   // garbage
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "push(int:63)");
  }
}

TEST_F(LexerTest, Popstk) {
  {
    std::vector<uint8_t> raw{
        0x03,                         // pop
        0x0a, 0x00, 0x00, 0x00, 0x00  // int
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "pop<int>()");
  }
}

TEST_F(LexerTest, ElmMarker) {
  std::vector<uint8_t> raw{0x08};

  auto result = lex.Parse(vec_to_sv(raw));
  EXPECT_EQ(result->ToDebugString(), "push(<elm>)");
}

TEST_F(LexerTest, Command) {
  std::vector<uint8_t> raw{
      0x30,                    // cmd
      0x01, 0x00, 0x00, 0x00,  // 1
      0x02, 0x00, 0x00, 0x00,  // 2
      0x03, 0x00, 0x00, 0x00,  // 3
      0x04, 0x00, 0x00, 0x00,  // 4
      0x05, 0x06, 0x07, 0x08,  // garbage
  };

  auto result = lex.Parse(vec_to_sv(raw));
  EXPECT_EQ(result->ToDebugString(), "cmd(1,2,3,4)");
}

TEST_F(LexerTest, PropertyExpand) {
  std::vector<uint8_t> raw{
      0x05,       // prop
      0x02, 0x0a  // garbage
  };

  auto result = lex.Parse(vec_to_sv(raw));
  EXPECT_EQ(result->ToDebugString(), "<prop>");
}

TEST_F(LexerTest, Operator2) {
  {
    std::vector<uint8_t> raw{
        0x22,                    // op2
        0x0a, 0x00, 0x00, 0x00,  // int
        0x0a, 0x00, 0x00, 0x00,  // int
        0x10                     // equal
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "int == int");
  }
}

TEST_F(LexerTest, Goto) {
  {
    std::vector<uint8_t> raw{
        0x12,                   // goto_false
        0xcf, 0x00, 0x00, 0x00  // 207
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "goto_false(207)");
  }

  {
    std::vector<uint8_t> raw{
        0x11,                   // goto_true
        0x04, 0x00, 0x00, 0x00  // 4
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "goto_true(4)");
  }

  {
    std::vector<uint8_t> raw{
        0x10,                   // goto
        0x10, 0x00, 0x00, 0x00  // 16
    };

    auto result = lex.Parse(vec_to_sv(raw));
    EXPECT_EQ(result->ToDebugString(), "goto(16)");
  }
}

TEST_F(LexerTest, Assign) {
  std::vector<uint8_t> raw{
      0x20,                    // assign
      0x0d, 0x00, 0x00, 0x00,  // (?)
      0x0a, 0x00, 0x00, 0x00,  // int
      0x01, 0x00, 0x00, 0x00   // 1
  };

  auto result = lex.Parse(vec_to_sv(raw));
  EXPECT_EQ(result->ToDebugString(), "let(1) unknown := int");
}
