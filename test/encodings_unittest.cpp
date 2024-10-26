// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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

#include "encodings/utf16.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

TEST(UTF16LEDecodeTest, Empty) {
  const std::string expected;

  {
    std::string_view input;
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }

  {
    std::vector<uint8_t> input;
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }

  {
    std::u16string input;
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }
}

TEST(UTF16LEDecodeTest, Ascii) {
  const std::string expected = "Hello";

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x48, 0x00,  // H
        0x65, 0x00,  // e
        0x6C, 0x00,  // l
        0x6C, 0x00,  // l
        0x6F, 0x00   // o
    };
    std::string_view input(reinterpret_cast<const char*>(utf16le_bytes.data()),
                           utf16le_bytes.size());
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x48, 0x00,  // H
        0x65, 0x00,  // e
        0x6C, 0x00,  // l
        0x6C, 0x00,  // l
        0x6F, 0x00   // o
    };

    std::string output = utf16le::Decode(utf16le_bytes);
    EXPECT_EQ(output, expected);
  }

  {
    std::u16string input = u"Hello";
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }
}

TEST(UTF16LEDecodeTest, Japanese) {
  const std::string expected = u8"こんにちは";

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x53, 0x30,  // こ
        0x93, 0x30,  // ん
        0x6B, 0x30,  // に
        0x61, 0x30,  // ち
        0x6F, 0x30   // は
    };

    std::string_view input(reinterpret_cast<const char*>(utf16le_bytes.data()),
                           utf16le_bytes.size());
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x53, 0x30,  // こ
        0x93, 0x30,  // ん
        0x6B, 0x30,  // に
        0x61, 0x30,  // ち
        0x6F, 0x30   // は
    };

    std::string output = utf16le::Decode(utf16le_bytes);
    EXPECT_EQ(output, expected);
  }

  {
    std::u16string input = u"こんにちは";
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }
}

TEST(UTF16LEDecodeTest, Emoji) {
  const std::string expected = u8"😀";

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x3D, 0xD8,  // High surrogate
        0x00, 0xDE   // Low surrogate
    };

    std::string_view input(reinterpret_cast<const char*>(utf16le_bytes.data()),
                           utf16le_bytes.size());
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }

  {
    std::vector<uint8_t> utf16le_bytes = {
        0x3D, 0xD8,  // High surrogate
        0x00, 0xDE   // Low surrogate
    };

    std::string output = utf16le::Decode(utf16le_bytes);
    EXPECT_EQ(output, expected);
  }

  {
    std::u16string input;
    input.push_back(0xD83D);  // High surrogate
    input.push_back(0xDE00);  // Low surrogate
    std::string output = utf16le::Decode(input);
    EXPECT_EQ(output, expected);
  }
}
