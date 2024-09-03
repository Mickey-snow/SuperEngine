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

#include "lru_cache.hpp"

TEST(LRUCacheTest, InitialSize) {
  LRUCache<int, int> cache(7);
  EXPECT_EQ(cache.size(), 0);
}

TEST(LRUCacheTest, InsertElements) {
  LRUCache<int, std::string> cache(3);
  cache.insert(1, "one");
  cache.insert(2, "two");
  EXPECT_EQ(cache.size(), 2);

  cache.insert(3, "three");
  EXPECT_EQ(cache.size(), 3);

  cache.insert(4, "four");
  EXPECT_EQ(cache.size(), 3)
      << "Exceeding max size should remove the least recently used element";
  EXPECT_FALSE(cache.exists(1)) << " '1' should have been evicted";
}

TEST(LRUCacheTest, FetchAndTouch) {
  LRUCache<int, std::string> cache(3);
  cache.insert(1, "one");
  cache.insert(2, "two");
  cache.insert(3, "three");

  // Access '1' to make it the most recently used
  EXPECT_EQ(cache.fetch(1), "one");

  cache.insert(4, "four");
  EXPECT_TRUE(cache.exists(1)) << "Inserting '4' should evict '2', not '1'";
  EXPECT_FALSE(cache.exists(2)) << "Inserting '4' should evict '2'";
  EXPECT_TRUE(cache.exists(3)) << "Inserting '4' should evict '2', not '3'";
  EXPECT_TRUE(cache.exists(4));
}

TEST(LRUCacheTest, RemoveElements) {
  LRUCache<int, int> cache(3);
  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30);

  cache.remove(2);
  EXPECT_EQ(cache.size(), 2);
  EXPECT_FALSE(cache.exists(2));
  EXPECT_TRUE(cache.exists(1));
  EXPECT_TRUE(cache.exists(3));

  cache.remove(1);
  cache.remove(3);
  EXPECT_EQ(cache.size(), 0);
}

TEST(LRUCacheTest, ClearCache) {
  LRUCache<int, double> cache(3);
  cache.insert(1, 1.1);
  cache.insert(2, 2.2);
  cache.insert(3, 3.3);
  EXPECT_EQ(cache.size(), 3);

  cache.clear();
  EXPECT_EQ(cache.size(), 0);
  EXPECT_FALSE(cache.exists(1));
  EXPECT_FALSE(cache.exists(2));
  EXPECT_FALSE(cache.exists(3));
}

TEST(LRUCacheTest, FetchPointer) {
  LRUCache<int, int> cache(3);
  cache.insert(1, 10);
  int* data_ptr = cache.fetch_ptr(1);
  ASSERT_NE(data_ptr, nullptr);
  EXPECT_EQ(*data_ptr, 10);

  *data_ptr = 20;
  EXPECT_EQ(cache.fetch(1), 20)
      << "modifying the pointer should modifies the cache";
}

TEST(LRUCacheTest, FetchNonExisting) {
  LRUCache<int, int> cache(3);
  EXPECT_EQ(cache.fetch(999), int())
      << "Fetch non-existing element should return default constructed int "
         "(0)";
  EXPECT_EQ(cache.fetch_ptr(999), nullptr)
      << "Fetch non-existing element pointer should return nullptr";
}

TEST(LRUCacheTest, InsertDuplicateKeys) {
  LRUCache<int, int> cache(2);
  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(1, 15);
  cache.insert(3, 99);

  ASSERT_TRUE(cache.exists(1));
  EXPECT_EQ(cache.fetch(1), 15);
  EXPECT_EQ(cache.size(), 2);
}

TEST(LRUCacheTest, GetAllKeys) {
  LRUCache<int, int> cache(3);
  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30);

  auto keys = cache.get_all_keys();
  EXPECT_EQ(keys.size(), 3);
  EXPECT_EQ(keys[0], 3) << "Most recently used should be first";
  EXPECT_EQ(keys[1], 2);
  EXPECT_EQ(keys[2], 1);
}
