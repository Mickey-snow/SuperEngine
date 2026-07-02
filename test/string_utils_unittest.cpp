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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "utilities/string_utilities.hpp"

#include <ranges>
#include <string>
#include <vector>

TEST(StringUtilTest, Join) {
  std::vector<int> arr{1, 2, 3};
  EXPECT_EQ(Join(", ", arr | std::views::transform(
                                 [](int x) { return std::to_string(x); })),
            "1, 2, 3");
}

TEST(StringUtilTest, RemoveQuotes) {
  EXPECT_EQ(RemoveQuotes("\"hello\""), "hello");
  EXPECT_EQ(RemoveQuotes("hello"), "hello");
  EXPECT_EQ(RemoveQuotes(""), "");
  EXPECT_EQ(RemoveQuotes("\"a\"b\""), "a\"b");
}

TEST(StringUtilTest, SiglusRegression) {
  int result;
  ASSERT_TRUE(parse_int("00123", result));
  EXPECT_EQ(result, 123);
  ASSERT_TRUE(parse_int("+7", result));
  EXPECT_EQ(result, 7);
  ASSERT_FALSE(parse_int("CNT", result));
}

TEST(StringUtilTest, Utf8CodepointHelpers) {
  const std::string hiragana_a = "\xe3\x81\x82";
  const std::string halfwidth_ka = "\xef\xbd\xb6";
  const std::string mixed = std::string("A") + hiragana_a + halfwidth_ka + "B";

  EXPECT_EQ(Utf8CodepointCount(mixed), 4u);
  EXPECT_EQ(Utf8ByteOffsetAtCodepointIndex(mixed, 0), 0u);
  EXPECT_EQ(Utf8ByteOffsetAtCodepointIndex(mixed, 1), 1u);
  EXPECT_EQ(Utf8ByteOffsetAtCodepointIndex(mixed, 2), 4u);
  EXPECT_EQ(Utf8ByteOffsetAtCodepointIndex(mixed, 99), mixed.size());
  EXPECT_EQ(Utf8ByteOffsetAfterCodepoints(mixed, 1, 2), 7u);
  EXPECT_EQ(Utf8CodepointIndexAtByteOffset(mixed, 4), 2u);
  EXPECT_EQ(Utf8CodepointIndexAtByteOffset(mixed, mixed.size()), 4u);

  EXPECT_EQ(Utf8CodepointAt(mixed, 0), 'A');
  EXPECT_EQ(Utf8CodepointAt(mixed, 1), 0x3042);
  EXPECT_EQ(Utf8CodepointAt(mixed, 2), 0xff76);
  EXPECT_EQ(Utf8CodepointAt(mixed, 4), std::nullopt);

  EXPECT_EQ(Utf8SubstringByCodepoints(mixed, 1, 2), hiragana_a + halfwidth_ka);
  EXPECT_EQ(Utf8SubstringByCodepoints(mixed, 2), halfwidth_ka + "B");
}

TEST(StringUtilTest, SiglusDisplayWidthHelpers) {
  const std::string hiragana_a = "\xe3\x81\x82";
  const std::string halfwidth_ka = "\xef\xbd\xb6";
  const std::string mixed = std::string("A") + hiragana_a + halfwidth_ka + "B";

  EXPECT_EQ(SiglusDisplayWidth(mixed), 5u);
  EXPECT_EQ(SiglusPrefixByDisplayWidth(mixed, 2), "A");
  EXPECT_EQ(SiglusPrefixByDisplayWidth(mixed, 3),
            std::string("A") + hiragana_a);
  EXPECT_EQ(SiglusSuffixByDisplayWidth(mixed, 3), halfwidth_ka + "B");
  EXPECT_EQ(SiglusSuffixByDisplayWidth(mixed, 4),
            hiragana_a + halfwidth_ka + "B");
  EXPECT_EQ(SiglusSubstringByDisplayWidth(mixed, 1, 1), "");
  EXPECT_EQ(SiglusSubstringByDisplayWidth(mixed, 1, 2), hiragana_a);
  EXPECT_EQ(SiglusSubstringByDisplayWidth(mixed, 2, 2), halfwidth_ka + "B");
}

TEST(StringUtilTest, AsciiCaseAndUtf8IndexSearch) {
  EXPECT_EQ(AsciiUpper("AbC xyz 123"), "ABC XYZ 123");
  EXPECT_EQ(AsciiLower("AbC XYZ 123"), "abc xyz 123");

  auto ascii_index = FindAsciiCaseInsensitiveUtf8Index("AbcAbCA", "bca", false);
  ASSERT_TRUE(ascii_index);
  EXPECT_EQ(*ascii_index, 1u);

  auto ascii_rindex = FindAsciiCaseInsensitiveUtf8Index("AbcAbCA", "bca", true);
  ASSERT_TRUE(ascii_rindex);
  EXPECT_EQ(*ascii_rindex, 4u);

  const std::string hiragana_a = "\xe3\x81\x82";
  const std::string halfwidth_ka = "\xef\xbd\xb6";
  const std::string mixed = std::string("A") + hiragana_a + halfwidth_ka + "B";
  auto utf8_index =
      FindAsciiCaseInsensitiveUtf8Index(mixed, halfwidth_ka, false);
  ASSERT_TRUE(utf8_index);
  EXPECT_EQ(*utf8_index, 2u);
  EXPECT_FALSE(FindAsciiCaseInsensitiveUtf8Index(mixed, "missing", false));
}
