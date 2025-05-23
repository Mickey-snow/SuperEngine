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

#include <thread>

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

TEST(LRUCacheTest, FetchOrElse) {
  LRUCache<int, std::string> cache(3);
  cache.insert(1, "one");
  cache.insert(2, "two");

  bool factory_called = false;
  auto default_factory = [&factory_called]() -> std::string {
    factory_called = true;
    return "default";
  };

  std::string result = cache.fetch_or_else(1, default_factory);
  EXPECT_EQ(result, "one");
  EXPECT_FALSE(factory_called)
      << "Default factory should not be called for existing keys";

  result = cache.fetch_or_else(3, default_factory);
  EXPECT_EQ(result, "default");
  EXPECT_TRUE(factory_called);

  result = cache.fetch(3);
  EXPECT_EQ(result, "default") << "Should be inserted into the cache";
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

class LRUCacheThreadSafetyTest : public ::testing::Test {
 protected:
  using ThreadSafeLRUCache = LRUCache<int, int, ThreadingModel::MultiThreaded>;
};

TEST_F(LRUCacheThreadSafetyTest, ConcurrentInsertions) {
  const int cache_size = 100;
  ThreadSafeLRUCache cache(cache_size);

  const int num_threads = 10;
  const int operations_per_thread = 1000;

  auto insert_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = thread_id * operations_per_thread + i;
      cache.insert(key, key);
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(insert_task, t);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_LE(cache.size(), cache_size)
      << "the cache size should be limited, it should not exceed cache_size";

  auto keys = cache.get_all_keys();
  EXPECT_EQ(keys.size(), cache.size());
}

TEST_F(LRUCacheThreadSafetyTest, ConcurrentFetchAndInsert) {
  const int cache_size = 100;
  ThreadSafeLRUCache cache(cache_size);

  const int num_threads = 10;
  const int operations_per_thread = 1000;

  for (int i = 0; i < cache_size; ++i) {
    cache.insert(i, i * 10);
  }

  auto fetch_and_insert_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = i % cache_size;
      cache.fetch(key);
      cache.insert(key + thread_id * operations_per_thread, key);
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(fetch_and_insert_task, t);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_LE(cache.size(), cache_size)
      << "the cache size should not exceed the maximum";
}

TEST_F(LRUCacheThreadSafetyTest, ConcurrentInsertAndRemove) {
  const int cache_size = 100;
  ThreadSafeLRUCache cache(cache_size);

  const int num_threads = 10;
  const int operations_per_thread = 1000;

  auto insert_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = thread_id * operations_per_thread + i;
      cache.insert(key, key);
    }
  };

  auto remove_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = thread_id * operations_per_thread + i;
      cache.remove(key);
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(insert_task, t);
    threads.emplace_back(remove_task, t);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_LE(cache.size(), cache_size);
}

TEST_F(LRUCacheThreadSafetyTest, ConcurrentMixedOperations) {
  const int cache_size = 1000;
  ThreadSafeLRUCache cache(cache_size);

  const int num_threads = 10;
  const int operations_per_thread = 1000;

  auto mixed_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = (thread_id + i) % (cache_size * 2);
      cache.insert(key, key);

      auto data = cache.fetch_or(key, key);
      EXPECT_EQ(data, key);

      if (i % 10 == 0) {
        cache.remove(key);
      }
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(mixed_task, t);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_LE(cache.size(), cache_size);
}

TEST_F(LRUCacheThreadSafetyTest, HighContention) {
  GTEST_SKIP();
  const int cache_size = 10;
  ThreadSafeLRUCache cache(cache_size);

  const int num_threads = 50;
  const int operations_per_thread = 10000;

  auto contention_task = [&cache, operations_per_thread](int thread_id) {
    for (int i = 0; i < operations_per_thread; ++i) {
      int key = i % cache_size;
      cache.insert(key, thread_id);
      cache.fetch(key);
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(contention_task, t);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_LE(cache.size(), cache_size);
  for (int key = 0; key < cache_size; ++key) {
    if (cache.exists(key)) {
      int data = cache.fetch(key);
      EXPECT_GE(data, 0);
      EXPECT_LT(data, num_threads);
    }
  }
}
