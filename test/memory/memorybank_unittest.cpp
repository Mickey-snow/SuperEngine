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

#include <map>
#include <string>

TEST(DynamicBankTest, Basic) {
  {
    IntBankStorage bank;
    EXPECT_EQ(bank.GetSize(), 0);
  }

  {
    IntBankStorage bank;
    bank.Resize(10);
    EXPECT_EQ(bank.GetSize(), 10);
  }

  {
    IntBankStorage bank;
    bank.Resize(10);
    bank.Set(0, 42);
    bank.Set(9, 99);
    EXPECT_EQ(bank.Get(0), 42);
    EXPECT_EQ(bank.Get(9), 99);
  }

  {
    StrBankStorage bank;
    bank.Resize(3);
    bank.Set(0, "Hello");
    bank.Set(1, "World");
    EXPECT_EQ(bank.Get(0), "Hello");
    EXPECT_EQ(bank.Get(1), "World");
  }
}

TEST(DynamicBankTest, GrowsOnOutOfBoundsAccess) {
  IntBankStorage bank;
  bank.Resize(5);

  bank.Set(5, 10);
  EXPECT_EQ(bank.GetSize(), 6);
  EXPECT_EQ(bank.Get(5), 10);

  EXPECT_EQ(bank.Get(8), 0);
  EXPECT_EQ(bank.GetSize(), 9);

  StrBankStorage strings;
  strings.Resize(2);
  EXPECT_EQ(strings.Get(3), "");
  EXPECT_EQ(strings.GetSize(), 4);
}

TEST(DynamicBankTest, SubwordWriteValidatesBeforeMutating) {
  IntBankStorage bank;
  bank.Resize(1);
  bank.Set(0, 0x12345678);

  EXPECT_THROW(bank.Set(0, 0b10000, 4), std::overflow_error);
  EXPECT_EQ(bank.Get(0), 0x12345678);

  EXPECT_THROW(bank.Set(0, -1, 4), std::overflow_error);
  EXPECT_EQ(bank.Get(0), 0x12345678);
}

TEST(DynamicBankTest, FillValues) {
  IntBankStorage bank;
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
  IntBankStorage bank;
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
  IntBankStorage bank;
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

TEST(DynamicBankTest, Serialization) {
  const size_t size = 128;
  std::stringstream ss;

  {
    StrBankStorage arr;
    arr.Resize(size);
    arr.Set(0, "zero");
    arr.Fill(16, 32, "chunk");
    arr.Set(size - 1, "last");

    boost::archive::text_oarchive oa(ss);
    oa << arr;
  }

  {
    boost::archive::text_iarchive ia(ss);
    StrBankStorage deserialized;
    ia >> deserialized;

    ASSERT_EQ(deserialized.GetSize(), size);
    EXPECT_EQ(deserialized.Get(0), "zero");
    EXPECT_EQ(deserialized.Get(15), "");
    EXPECT_EQ(deserialized.Get(16), "chunk");
    EXPECT_EQ(deserialized.Get(31), "chunk");
    EXPECT_EQ(deserialized.Get(32), "");
    EXPECT_EQ(deserialized.Get(size - 1), "last");
  }
}

TEST(DynamicBankTest, DeserializationReplacesExistingStorage) {
  std::stringstream ss;
  {
    IntBankStorage source;
    source.Resize(10);
    source.Set(2, 22);

    boost::archive::text_oarchive oa(ss);
    oa << source;
  }

  IntBankStorage target;
  target.Resize(20);
  target.Set(15, 99);

  {
    boost::archive::text_iarchive ia(ss);
    ia >> target;
  }

  ASSERT_EQ(target.GetSize(), 10);
  EXPECT_EQ(target.Get(2), 22);

  target.Resize(20);
  EXPECT_EQ(target.Get(15), 0);
}
