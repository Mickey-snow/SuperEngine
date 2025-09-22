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
  ASSERT_FALSE(parse_int("CNT", result));
}
