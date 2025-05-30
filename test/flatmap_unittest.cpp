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

#include "utilities/flat_map.hpp"

#include <optional>
#include <stdexcept>
#include <string>

TEST(FlatMapTest, Default) {
  flat_map<int> fmap(0, 3);
  for (int key = 0; key <= 3; ++key)
    EXPECT_FALSE(fmap.contains(key));

  // Out-of-range contains should be false, not throw
  EXPECT_FALSE(fmap.contains(-1));
  EXPECT_FALSE(fmap.contains(4));
}

TEST(FlatMapTest, Insert) {
  flat_map<std::string> fmap(1, 3);
  fmap.insert(1, std::string("one"));
  fmap.insert(3, "three");
  EXPECT_TRUE(fmap.contains(1));
  EXPECT_TRUE(fmap.contains(3));
  EXPECT_EQ(fmap.at(1), "one");
  EXPECT_EQ(fmap.at(3), "three");
  // operator[] returns optional reference
  EXPECT_TRUE(fmap[1].has_value());
  EXPECT_EQ(*fmap[1], "one");
}

TEST(FlatMapTest, AtEmpty) {
  flat_map<int> fmap(5, 7);
  EXPECT_THROW(fmap.at(5), std::out_of_range);
  EXPECT_THROW(fmap.at(6), std::out_of_range);
}

// Test out-of-range behavior
TEST(FlatMapTest, OutOfRange) {
  flat_map<int> fmap(0, 1);
  EXPECT_THROW(fmap.insert(-1, 10), std::out_of_range);
  EXPECT_THROW(fmap.insert(2, 20), std::out_of_range);
  EXPECT_THROW(fmap.at(-5), std::out_of_range);
  EXPECT_THROW(fmap.at(5), std::out_of_range);
  EXPECT_THROW(fmap[-5], std::out_of_range);
  EXPECT_THROW(fmap[-5], std::out_of_range);
}

// Test clear and emplace
TEST(FlatMapTest, ClearEmplace) {
  flat_map<int> fmap(-1, 1);
  fmap.emplace(0, 123);
  EXPECT_TRUE(fmap.contains(0));
  EXPECT_EQ(fmap.at(0), 123);
  fmap.clear();
  EXPECT_FALSE(fmap.contains(0));
}

TEST(FlatMapTest, MakeMap) {
  auto fmap = make_flatmap<char>(
      {std::pair{10, 'a'}, std::pair{12, 'c'}, std::pair{11, 'b'}});
  EXPECT_TRUE(fmap.contains(10));
  EXPECT_TRUE(fmap.contains(11));
  EXPECT_TRUE(fmap.contains(12));
  EXPECT_EQ(fmap.at(10), 'a');
  EXPECT_EQ(fmap.at(11), 'b');
  EXPECT_EQ(fmap.at(12), 'c');
}
