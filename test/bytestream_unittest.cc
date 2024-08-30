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

#include "utilities/bytestream.h"

#include <string>
#include <string_view>

TEST(ByteStreamTest, ReadInt) {
  char raw[] = {0x72, 0x98, 0xa1, 0xc9};
  static constexpr auto N = sizeof(raw) / sizeof(char);

  ByteStream bs(raw, N);
  EXPECT_EQ(bs.ReadBytes(0), 0);
  EXPECT_EQ(bs.ReadBytes(1), 0x72);
  EXPECT_EQ(bs.ReadBytes(2), 0x9872);
  EXPECT_EQ(bs.ReadBytes(3), 0xa19872);
  EXPECT_EQ(bs.ReadBytes(4), 0xc9a19872);
  EXPECT_THROW(bs.ReadBytes(5), std::out_of_range);
}

TEST(ByteStreamTest, PopInt) {
  char raw[] = {0xab, 0x2d, 0x12, 0x33, 0x9a, 0xff,
                0xf1, 0xfb, 0x7f, 0x46, 0xa9, 0x8c};
  static constexpr auto N = sizeof(raw) / sizeof(char);

  ByteStream bs(raw, N);
  EXPECT_EQ(bs.PopBytes(0), 0);
  EXPECT_EQ(bs.PopBytes(8), 0xfbf1ff9a33122dabull);
  bs.Proceed(2);
  EXPECT_EQ(bs.PopBytes(2), 0x8ca9);
  EXPECT_EQ(bs.PopBytes(0), 0);
  EXPECT_THROW(bs.PopBytes(1), std::out_of_range);
}

TEST(ByteStreamTest, Seek) {
  char raw[] = {0xab, 0x2d, 0x12, 0x33, 0x9a, 0xff,
                0xf1, 0xfb, 0x7f, 0x46, 0xa9, 0x8c};
  static constexpr auto N = sizeof(raw) / sizeof(char);

  ByteStream bs(raw, raw + N);
  EXPECT_EQ(bs.Size(), N);
  bs.PopBytes(6);
  EXPECT_EQ(bs.Position(), 6);
  bs.Seek(2);
  EXPECT_EQ(bs.PopBytes(3), 0x9a3312);
  EXPECT_EQ(bs.Position(), 5);
  bs.Proceed(-5);
  EXPECT_EQ(bs.Position(), 0);
  EXPECT_THROW(bs.Seek(N + 1), std::out_of_range);
}

TEST(ByteStreamTest, ReadAs) {
  char raw[] = {0x90, 0xbe, 0xa7, 0xb3, 0xff, 0xa1, 0xcd, 0x4,
                0xcc, 0x33, 0xee, 0xe6, 0xa1, 0x0f, 0x44, 0x0f};
  static constexpr auto N = sizeof(raw) / sizeof(char);

  ByteStream bs(raw, raw + N);
  EXPECT_EQ(bs.ReadAs<uint16_t>(2), 48784);
  EXPECT_EQ(bs.PopAs<int16_t>(2), -16752);

  long long ll_value;
  unsigned long long ull_value;
  bs.Seek(4);
  bs >> ll_value;
  bs.Proceed(-8);
  bs >> ull_value;
  EXPECT_EQ(ll_value, -1806449449182060033ll);
  EXPECT_EQ(ull_value, 16640294624527491583ull);

  float flt_value;
  bs.Seek(11);
  bs >> flt_value;
  EXPECT_FLOAT_EQ(flt_value, 574.5297);
}

TEST(ByteStreamTest, Strings) {
  using std::string_literals::operator""s;
  using std::string_view_literals::operator""sv;

  char raw[] = "Hello, World!";
  static constexpr auto N = sizeof(raw) / sizeof(char);

  ByteStream bs(raw, raw + N);
  EXPECT_EQ(bs.PopAs<char>(1), 'H');
  EXPECT_EQ(bs.ReadAs<std::string>(6), "ello, "s);
  EXPECT_EQ(bs.PopAs<std::string_view>(6), "ello, "sv);
  EXPECT_EQ(bs.Position(), 7);
}
