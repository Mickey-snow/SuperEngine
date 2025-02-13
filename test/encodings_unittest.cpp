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

#include "encodings/codepage.hpp"
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
  const auto data = u8"こんにちは";
  const std::string expected(reinterpret_cast<char const*>(data));

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
  const auto data = u8"😀";
  const std::string expected(reinterpret_cast<char const*>(data));

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

TEST(CodepageTest, Cp1252Conversion) {
  // Create a cp1252 converter
  auto converter = Codepage::Create(Encoding::cp1252);
  ASSERT_NE(converter, nullptr) << "Failed to create cp1252 converter";

  // In cp1252, the string "café" may be encoded as "caf\xe9"
  std::string input = "caf\xe9";
  // Expected UTF-8 conversion: "caf\xc3\xa9"
  std::string expected = "caf\xc3\xa9";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, Cp932Conversion) {
  auto converter = Codepage::Create(Encoding::cp932);
  ASSERT_NE(converter, nullptr) << "Failed to create cp932 converter";

  std::string input = "\x93\xfa\x96\x7b";
  std::string expected = "\xE6\x97\xA5\xE6\x9C\xAC";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, Cp936Conversion) {
  auto converter = Codepage::Create(Encoding::cp936);
  ASSERT_NE(converter, nullptr) << "Failed to create cp936 converter";

  std::string input = "\xC4\xE3\xBA\xC3";
  std::string expected = "\xE4\xBD\xA0\xE5\xA5\xBD";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, Cp949Conversion) {
  auto converter = Codepage::Create(Encoding::cp949);
  ASSERT_NE(converter, nullptr) << "Failed to create cp949 converter";

  std::string input = "\xc7\xd1\xb1\xdb";
  std::string expected = "\xED\x95\x9C\xEA\xB8\x80";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, EmptyInput) {
  auto converter = Codepage::Create(Encoding::cp1252);
  ASSERT_NE(converter, nullptr);

  std::string input = "";
  std::string expected = "";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, AsciiInput) {
  auto converter = Codepage::Create(Encoding::cp1252);
  ASSERT_NE(converter, nullptr);

  std::string input = "Hello, World!";
  std::string expected = "Hello, World!";
  EXPECT_EQ(converter->ConvertTo_utf8(input), expected);
}

TEST(CodepageTest, RegnameRegression) {
  auto converter = Codepage::Create(Encoding::cp932);
  ASSERT_NE(converter, nullptr);

  EXPECT_EQ(
      converter->ConvertTo_utf8("\x4b\x45\x59\x5c\x83\x8a\x83\x67\x83\x8b\x83"
                                "\x6f\x83\x58\x83\x5e\x81\x5b\x83\x59\x81\x49"),
      "KEY\\リトルバスターズ！");
  EXPECT_EQ(converter->ConvertTo_utf8(
                "\x4b\x45\x59\x5c\x83\x8a\x83\x67\x83\x8b\x83\x6f\x83\x58\x83"
                "\x5e\x81\x5b\x83\x59\x81\x49\x82\x64\x82\x77"),
            "KEY\\リトルバスターズ！ＥＸ");
  EXPECT_EQ(
      converter->ConvertTo_utf8(
          "\x4b\x45\x59\x5c\x83\x4e\x83\x68\x82\xed\x82\xd3\x82\xbd\x81\x5b"),
      "KEY\\クドわふたー");

  EXPECT_EQ(converter->ConvertTo_utf8(
                "\x4b\x45\x59\x5c\x83\x4e\x83\x68\x82\xed\x82\xd3\x82"
                "\xbd\x81\x5b\x81\x79\x91\x53\x94\x4e\x97\xee\x91\xce"
                "\x8f\xdb\x94\xc5\x81\x7a"),
            "KEY\\クドわふたー【全年齢対象版】");

  EXPECT_EQ(converter->ConvertTo_utf8("KEY\\CLANNAD_FV"), "KEY\\CLANNAD_FV");
  EXPECT_EQ(converter->ConvertTo_utf8("StudioMebius\\SNOWSE"),
            "StudioMebius\\SNOWSE");

  EXPECT_EQ(
      "\xe3\x83\x9e\xe3\x82\xb8\xef\xbc\x9f\xe3\x80\x80\xe3\x81\x84\xe3\x81\x84"
      "\xe3\x81\xae\xe3\x81\x8b\xef\xbc\x9f",
      converter->ConvertTo_utf8(
          "\x83\x7d\x83\x57\x81\x48\x81\x40\x82\xa2\x82\xa2\x82\xcc\x82\xa9\x81"
          "\x48"))
      << "Didn't convert the string 'maji? iinoka?' correctly.";
}
