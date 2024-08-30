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

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

TEST(oBytestreamTest, basic) {
  oBytestream obs;

  struct Header {
    int foo = 1234;
    long boo = 2345;
    bool flag = 1;
  } myheader;
  obs << "RIFF" << 1234 << "WAVEfmt " << myheader;

  std::vector<uint8_t> result = obs.Get();
}

TEST(oBytestreamTest, InsertBasicTypes) {
  oBytestream stream;

  uint8_t u8 = 255;
  int32_t i32 = -123456;
  double d = 3.141592653589793;

  stream << u8 << i32 << d;

  const auto& buffer = stream.Get();
  ASSERT_EQ(buffer.size(), sizeof(u8) + sizeof(i32) + sizeof(d));

  uint8_t u8_result;
  int32_t i32_result;
  double d_result;

  std::memcpy(&u8_result, buffer.data(), sizeof(u8));
  std::memcpy(&i32_result, buffer.data() + sizeof(u8), sizeof(i32));
  std::memcpy(&d_result, buffer.data() + sizeof(u8) + sizeof(i32), sizeof(d));

  EXPECT_EQ(u8_result, u8);
  EXPECT_EQ(i32_result, i32);
  EXPECT_DOUBLE_EQ(d_result, d);
}

TEST(oBytestreamTest, InsertStrings) {
  using std::string_literals::operator""s;
  using std::string_view_literals::operator""sv;

  oBytestream stream;
  std::string str = "Hello"s;
  std::string_view sv = " World"sv;

  stream << str << sv << "! ";

  const auto& buffer = stream.Get();
  ASSERT_EQ(buffer.size(), str.size() + sv.size() + 2);

  std::string result(buffer.begin(), buffer.end());
  EXPECT_EQ(result, "Hello World! "s);
}

TEST(oBytestreamTest, BufferManipulation) {
  oBytestream stream;

  stream << int32_t(100);

  auto& buffer = stream.Get();
  EXPECT_EQ(buffer.size(), sizeof(int32_t));

  // Directly modify the buffer
  buffer.push_back(255);

  EXPECT_EQ(stream.Get().size(), sizeof(int32_t) + 1);
}

TEST(oBytestreamTest, FlushOperation) {
  oBytestream stream;

  stream << int32_t(100);
  ASSERT_FALSE(stream.Get().empty());

  stream.Flush();
  EXPECT_TRUE(stream.Get().empty());
}

TEST(oBytestreamTest, GetCopy) {
  oBytestream stream;

  stream << int32_t(100) << double(2.718);

  auto copy = stream.GetCopy();
  stream.Flush();
  ASSERT_EQ(copy.size(), sizeof(int32_t) + sizeof(double));

  int32_t i32_result;
  double d_result;

  std::memcpy(&i32_result, copy.data(), sizeof(int32_t));
  std::memcpy(&d_result, copy.data() + sizeof(int32_t), sizeof(double));

  EXPECT_EQ(i32_result, 100);
  EXPECT_DOUBLE_EQ(d_result, 2.718);
}

// Test case for inserting empty strings and large data types
TEST(oBytestreamTest, InsertEdgeCases) {
  oBytestream stream;

  std::string empty_str = "";
  stream << empty_str;

  EXPECT_TRUE(stream.Get().empty());

  int64_t large_value = 9223372036854775807LL;
  stream << large_value;

  const auto& buffer = stream.Get();
  ASSERT_EQ(buffer.size(), sizeof(int64_t));

  int64_t large_value_result;
  std::memcpy(&large_value_result, buffer.data(), sizeof(int64_t));

  EXPECT_EQ(large_value_result, large_value);
}
