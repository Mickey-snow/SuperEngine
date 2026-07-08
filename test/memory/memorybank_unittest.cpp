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

#include "core/memory_internal/proxy.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>
#include <string>
#include <vector>

TEST(DynamicBankTest, Basic) {
  {
    std::vector<int> storage;
    IntListProxy bank(storage);
    EXPECT_EQ(bank.GetSize(), 0);
  }

  {
    std::vector<int> storage;
    IntListProxy bank(storage);
    bank.Resize(10);
    EXPECT_EQ(bank.GetSize(), 10);
  }

  {
    std::vector<int> storage;
    IntListProxy bank(storage);
    bank.Resize(10);
    bank.Set(0, 42);
    bank.Set(9, 99);
    EXPECT_EQ(bank.Get(0), 42);
    EXPECT_EQ(bank.Get(9), 99);
  }

  {
    std::vector<std::string> storage;
    StrListProxy bank(storage);
    bank.Resize(3);
    bank.Set(0, "Hello");
    bank.Set(1, "World");
    EXPECT_EQ(bank.Get(0), "Hello");
    EXPECT_EQ(bank.Get(1), "World");
  }
}

TEST(DynamicBankTest, GrowsOnOutOfBoundsAccess) {
  std::vector<int> int_storage;
  IntListProxy bank(int_storage);
  bank.Resize(5);

  bank.Set(5, 10);
  EXPECT_EQ(bank.GetSize(), 6);
  EXPECT_EQ(bank.Get(5), 10);

  EXPECT_EQ(bank.Get(8), 0);
  EXPECT_EQ(bank.GetSize(), 9);

  std::vector<std::string> str_storage;
  StrListProxy strings(str_storage);
  strings.Resize(2);
  EXPECT_EQ(strings.Get(3), "");
  EXPECT_EQ(strings.GetSize(), 4);
}

TEST(DynamicBankTest, SubwordWriteValidatesBeforeMutating) {
  std::vector<int> storage;
  IntListProxy bank(storage);
  bank.Resize(1);
  bank.Set(0, 0x12345678);

  EXPECT_THROW(bank.Set(0, 0b10000, 4), std::overflow_error);
  EXPECT_EQ(bank.Get(0), 0x12345678);

  EXPECT_THROW(bank.Set(0, -1, 4), std::overflow_error);
  EXPECT_EQ(bank.Get(0), 0x12345678);
}

TEST(DynamicBankTest, FillValues) {
  std::vector<int> storage;
  IntListProxy bank(storage);
  bank.Resize(10);
  bank.Fill(2, 5, 7);
  for (size_t i = 2; i < 5; ++i) {
    EXPECT_EQ(bank.Get(i), 7);
  }
  EXPECT_NE(bank.Get(6), 7);

  EXPECT_NO_THROW(bank.Fill(0, 0, 9));
  EXPECT_EQ(bank.Get(0), 0);
  EXPECT_NO_THROW(bank.Fill(10, 10, 9));
  EXPECT_EQ(bank.GetSize(), 10);
  EXPECT_NO_THROW(bank.Fill(11, 11, 9));
  EXPECT_EQ(bank.GetSize(), 10);
  EXPECT_NO_THROW(bank.Fill(12, 15, 9));
  EXPECT_EQ(bank.GetSize(), 15);
  EXPECT_EQ(bank.Get(14), 9);
  EXPECT_THROW(bank.Fill(6, 5, 9), std::invalid_argument);
}

TEST(DynamicBankTest, Append) {
  std::vector<int> storage;
  IntListProxy bank(storage);
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

TEST(DynamicBankTest, Persistence) {
  std::vector<int> storage;
  IntListProxy bank(storage);
  bank.Resize(5);
  bank.Set(0, 1);
  auto memento1 = storage;
  bank.Set(1, 2);
  auto memento2 = storage;

  bank.Resize(1024);
  bank.Fill(7, 300, -10);
  bank.Fill(200, 500, 10);
  auto memento3 = storage;

  bank.Set(0, 42);
  EXPECT_EQ(bank.Get(0), 42);

  storage = memento3;
  EXPECT_EQ(bank.Get(0), 1);
  EXPECT_EQ(bank.Get(1), 2);
  EXPECT_EQ(bank.Get(99), -10);
  EXPECT_EQ(bank.Get(200), 10);

  IntListProxy memento2_proxy(memento2);
  EXPECT_EQ(memento2_proxy.Get(0), 1);
  EXPECT_EQ(memento2_proxy.Get(1), 2);

  storage = memento1;
  EXPECT_EQ(bank.GetSize(), 5);
  EXPECT_EQ(bank.Get(0), 1);
}

TEST(DynamicBankTest, Serialization) {
  const size_t size = 128;
  std::stringstream ss;

  {
    std::vector<std::string> arr;
    StrListProxy proxy(arr);
    proxy.Resize(size);
    proxy.Set(0, "zero");
    proxy.Fill(16, 32, "chunk");
    proxy.Set(size - 1, "last");

    boost::archive::text_oarchive oa(ss);
    oa << arr;
  }

  {
    boost::archive::text_iarchive ia(ss);
    std::vector<std::string> deserialized;
    ia >> deserialized;
    StrListProxy proxy(deserialized);

    ASSERT_EQ(proxy.GetSize(), size);
    EXPECT_EQ(proxy.Get(0), "zero");
    EXPECT_EQ(proxy.Get(15), "");
    EXPECT_EQ(proxy.Get(16), "chunk");
    EXPECT_EQ(proxy.Get(31), "chunk");
    EXPECT_EQ(proxy.Get(32), "");
    EXPECT_EQ(proxy.Get(size - 1), "last");
  }
}

TEST(DynamicBankTest, DeserializationReplacesExistingStorage) {
  std::stringstream ss;
  {
    std::vector<int> source;
    IntListProxy proxy(source);
    proxy.Resize(10);
    proxy.Set(2, 22);

    boost::archive::text_oarchive oa(ss);
    oa << source;
  }

  std::vector<int> target;
  IntListProxy proxy(target);
  proxy.Resize(20);
  proxy.Set(15, 99);

  {
    boost::archive::text_iarchive ia(ss);
    ia >> target;
  }

  ASSERT_EQ(proxy.GetSize(), 10);
  EXPECT_EQ(proxy.Get(2), 22);

  proxy.Resize(20);
  EXPECT_EQ(proxy.Get(15), 0);
}
