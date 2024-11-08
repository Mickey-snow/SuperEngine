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

#include "memory/bank.hpp"

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
