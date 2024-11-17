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

#include "utilities/byte_inserter.hpp"

TEST(ByteInserterTest, InsertUint8) {
  std::vector<uint8_t> buf;
  ByteInserter back(buf);

  uint8_t val = 12;
  back = val;
  ASSERT_EQ(buf.size(), 1);
  EXPECT_EQ(*buf.data(), val);
}

TEST(ByteInserterTest, InsertInt32) {
  std::vector<uint8_t> buf;

  int32_t expected[] = {9, -42, 70, 0, 38, -1};
  const size_t N = sizeof(expected) / sizeof(int32_t);
  std::copy_n(expected, N, ByteInserter(buf));
  EXPECT_EQ(memcmp(expected, buf.data(), sizeof(expected)), 0);
}

TEST(ByteInserterTest, InsertFloats) {
  std::vector<uint8_t> buf;

  float floats[] = {0.2384, 432.2213, 3.2856997337, 0, -12.4316097458};
  double doubles[] = {1.0874616057,   95e-60, -31.5320785225,
                      -42.8918018145, 59e55,  -108.8412894848};
  const size_t N1 = sizeof(floats) / sizeof(float);
  const size_t N2 = sizeof(doubles) / sizeof(double);
  std::copy_n(floats, N1, ByteInserter(buf));
  std::copy_n(doubles, N2, ByteInserter(buf));
  EXPECT_EQ(memcmp(floats, buf.data(), sizeof(floats)), 0);
  EXPECT_EQ(memcmp(doubles, buf.data() + sizeof(floats), sizeof(doubles)), 0);
}

TEST(ByteInserterTest, InsertMixedStructs) {
  std::vector<uint8_t> buf;
  ByteInserter back(buf);

  struct MixedStruct {
    int32_t int_val;
    float float_val;
    char char_val;
  };
  MixedStruct s1 = {1234, 5.67f, 'A'};
  MixedStruct s2 = {-4321, -8.91f, 'B'};

  back = s1;
  back = s2;

  MixedStruct* result1 = (MixedStruct*)buf.data();
  MixedStruct* result2 = (MixedStruct*)buf.data() + 1;

  EXPECT_EQ(result1->int_val, s1.int_val);
  EXPECT_FLOAT_EQ(result1->float_val, s1.float_val);
  EXPECT_EQ(result1->char_val, s1.char_val);

  EXPECT_EQ(result2->int_val, s2.int_val);
  EXPECT_FLOAT_EQ(result2->float_val, s2.float_val);
  EXPECT_EQ(result2->char_val, s2.char_val);
}

TEST(ByteInserterTest, InsertMixedUnions) {
  std::vector<uint8_t> buf;
  ByteInserter back(buf);

  union MixedUnion {
    MixedUnion() : int_val(0) {}
    int32_t int_val;
    float float_val;
    char char_val;
  };
  MixedUnion u1;
  u1.int_val = 1234;
  MixedUnion u2;
  u2.float_val = 5.67f;
  MixedUnion u3;
  u3.char_val = 'C';

  back = u1;
  back = u2;
  back = u3;

  MixedUnion result1;
  MixedUnion result2;
  MixedUnion result3;
  std::memcpy(&result1, buf.data(), sizeof(MixedUnion));
  std::memcpy(&result2, buf.data() + sizeof(MixedUnion), sizeof(MixedUnion));
  std::memcpy(&result3, buf.data() + 2 * sizeof(MixedUnion),
              sizeof(MixedUnion));

  EXPECT_EQ(result1.int_val, u1.int_val);
  EXPECT_FLOAT_EQ(result2.float_val, u2.float_val);
  EXPECT_EQ(result3.char_val, u3.char_val);
}

TEST(ByteInserterTest, InsertLargerDataTypes) {
  std::vector<uint8_t> buf;
  ByteInserter back(buf);

  int64_t large_int = 9223372036854775807LL;  // Max int64_t value
  double large_double = 3.141592653589793;

  back = large_int;
  back = large_double;

  int64_t result_int;
  double result_double;

  std::memcpy(&result_int, buf.data(), sizeof(int64_t));
  std::memcpy(&result_double, buf.data() + sizeof(int64_t), sizeof(double));

  EXPECT_EQ(result_int, large_int);
  EXPECT_DOUBLE_EQ(result_double, large_double);
}

TEST(ByteInserterTest, InsertStrings) {
  using std::string_literals::operator""s;
  using std::string_view_literals::operator""sv;

  std::vector<uint8_t> buf;
  ByteInserter back(buf);

  back = "Hello,"s;
  back = " World!"sv;
  back = "Hello, World";

  std::string str;
  str.resize(buf.size());
  std::memcpy(str.data(), buf.data(), buf.size());

  EXPECT_EQ(str, "Hello, World!Hello, World"s);
}
