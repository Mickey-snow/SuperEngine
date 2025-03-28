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

#include "core/memory.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <cmath>
#include <string>

TEST(MemoryBankTest, Basic) {
  {
    MemoryBank<int> bank;
    EXPECT_EQ(bank.GetSize(), 0);
  }

  {
    MemoryBank<int> bank;
    bank.Resize(10);
    EXPECT_EQ(bank.GetSize(), 10);
  }

  {
    MemoryBank<int> bank;
    bank.Resize(10);
    bank.Set(0, 42);
    bank.Set(9, 99);
    EXPECT_EQ(bank.Get(0), 42);
    EXPECT_EQ(bank.Get(9), 99);
  }

  {
    MemoryBank<std::string> bank;
    bank.Resize(3);
    bank.Set(0, "Hello");
    bank.Set(1, "World");
    EXPECT_EQ(bank.Get(0), "Hello");
    EXPECT_EQ(bank.Get(1), "World");
  }
}

TEST(MemoryBankTest, OutOfBounds) {
  MemoryBank<int> bank;
  bank.Resize(5);
  EXPECT_THROW(bank.Set(5, 10), std::out_of_range);
  EXPECT_THROW(bank.Get(5), std::out_of_range);
}

TEST(MemoryBankTest, FillValues) {
  MemoryBank<int> bank;
  bank.Resize(10);
  bank.Fill(2, 5, 7);
  for (size_t i = 2; i < 5; ++i) {
    EXPECT_EQ(bank.Get(i), 7);
  }
  EXPECT_NE(bank.Get(6), 7);
}

TEST(MemoryBankTest, Append) {
  MemoryBank<int> bank;
  for (int i = 0; i < 1000; ++i) {
    bank.Resize(i + 1);
    bank.Set(i, i);
  }

  for (int i = 1000 - 1; i >= 0; --i) {
    EXPECT_EQ(bank.Get(i), i);
    bank.Resize(i);
  }
  EXPECT_EQ(bank.GetSize(), 0);
}

TEST(MemoryBankTest, Persistence) {
  MemoryBank<int> bank;
  bank.Resize(5);
  bank.Set(0, 1);
  auto memento1 = bank;
  bank.Set(1, 2);
  auto memento2 = bank;

  bank.Resize(1024);
  bank.Fill(7, 300, -10);
  bank.Fill(200, 500, 10);
  auto memento3 = bank;

  bank.Set(0, 42);
  EXPECT_EQ(bank.Get(0), 42);

  bank = memento3;
  EXPECT_EQ(bank.Get(0), 1);
  EXPECT_EQ(bank.Get(1), 2);
  EXPECT_EQ(bank.Get(99), -10);
  EXPECT_EQ(bank.Get(200), 10);

  EXPECT_EQ(memento2.Get(0), 1);
  EXPECT_EQ(memento2.Get(1), 2);

  bank = memento1;
  EXPECT_EQ(bank.GetSize(), 5);
  EXPECT_EQ(bank.Get(0), 1);
}

TEST(MemoryBankTest, Serialization) {
  const size_t size = 100000;
  std::stringstream ss;
  size_t serialized_data_len = 0;

  {
    MemoryBank<std::string> arr;
    arr.Resize(size);
    for (size_t i = 0; i < 100; ++i) {
      // fill random data
      const auto value = std::to_string(i * i);
      arr.Set(i, value);
      serialized_data_len += value.length();
    }
    for (size_t i = 100; i < size;) {
      // fill with data chunk
      const size_t end = std::min(size, i + 1000);
      const auto value = std::to_string(i);
      arr.Fill(i, end, value);
      serialized_data_len += value.length();
      i = end + 1;
    }

    boost::archive::text_oarchive oa(ss);
    oa << arr;
  }

  EXPECT_LE(ss.tellp(), 4 * std::log2(size) * serialized_data_len);

  {
    boost::archive::text_iarchive ia(ss);
    MemoryBank<std::string> deserialized;
    ia >> deserialized;

    ASSERT_EQ(deserialized.GetSize(), 100000);
    for (size_t i = 0; i < 100; ++i)
      EXPECT_EQ(deserialized.Get(i), std::to_string(i * i));
  }
}

TEST(MemoryBankTest, Deserialization) {
  // Ensure implementations provide deserialization logic with compatibility
  std::stringstream ss(
      "22 serialization::archive 19 0 0 10 9 0 1 3 1 2 3 2 3 99 3 4 0 4 6 0 6 "
      "7 0 7 8 10 8 16 0 16 32 0");
  constexpr size_t size = 10;

  std::map<int, int> data{{0, 3}, {1, 3}, {2, 99}, {7, 10}};
  for (int i = 0; i < size; ++i)
    data.emplace(i, 0);

  MemoryBank<int> arr;
  boost::archive::text_iarchive ia(ss);
  ASSERT_NO_THROW({ ia >> arr; });

  ASSERT_EQ(arr.GetSize(), size);
  for (int i = 0; i < size; ++i) {
    EXPECT_EQ(arr.Get(i), data.at(i));
  }
}
