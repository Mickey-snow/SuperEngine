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

#include "memory/location.hpp"
#include "memory/memory.hpp"

#include <bitset>
#include <map>
#include <random>
#include <sstream>
#include <string>

class MemoryTest : public ::testing::Test {
 protected:
  MemoryTest() : memory_(std::make_unique<Memory>()) {}

  std::unique_ptr<Memory> memory_;
};

TEST_F(MemoryTest, Init) {
  for (size_t bank = 0; bank < static_cast<size_t>(IntBank::CNT); ++bank) {
    for (size_t index = 0; index < 2000; ++index) {
      IntMemoryLocation loc(static_cast<IntBank>(bank), index);
      EXPECT_EQ(memory_->Read(loc), 0) << "each IntBank should be initialized "
                                          "with size 2000 and default value 0";
    }
  }

  for (size_t bank = 0; bank < static_cast<size_t>(StrBank::CNT); ++bank) {
    for (size_t index = 0; index < 2000; ++index) {
      StrMemoryLocation loc(static_cast<StrBank>(bank), index);
      EXPECT_EQ(memory_->Read(loc), "")
          << "each StrBank should be initialized with "
             "size 2000 and default empty string";
    }
  }
}

TEST_F(MemoryTest, ReadWrite) {
  {
    IntMemoryLocation loc(IntBank::A, 100);

    memory_->Write(loc, 42);
    EXPECT_EQ(memory_->Read(loc), 42);

    IntMemoryLocation loc2(IntBank::Z, 101);
    EXPECT_EQ(memory_->Read(loc2), 0);

    IntMemoryLocation invalid_loc(IntBank::F, 2000);
    EXPECT_THROW(memory_->Read(invalid_loc), std::out_of_range);
    EXPECT_THROW(memory_->Write(invalid_loc, 10), std::out_of_range);
  }

  {
    StrMemoryLocation loc(StrBank::S, 150);

    memory_->Write(loc, "Hello World");
    EXPECT_EQ(memory_->Read(loc), "Hello World");

    StrMemoryLocation loc2(StrBank::M, 151);
    EXPECT_EQ(memory_->Read(loc2), "");

    StrMemoryLocation invalid_loc(StrBank::S, 2000);
    EXPECT_THROW(memory_->Read(invalid_loc), std::out_of_range);
    EXPECT_THROW(memory_->Write(invalid_loc, "Test"), std::out_of_range);
  }
}

TEST_F(MemoryTest, WriteInt) {
  auto Write = [&](int bit, int index, int value) {
    auto loc = IntMemoryLocation(IntBank::B, index, bit);
    memory_->Write(loc, value);
  };

  Write(2, 16, 0b01);
  Write(1, 35, 0b1);
  Write(8, 5, 0b10000101);
  Write(4, 9, 0b101);
  Write(16, 3, 0b0100110110011100);

  EXPECT_EQ(memory_->Read(IntMemoryLocation(IntBank::B, 1)), 1302103385)
      << std::bitset<32>(memory_->Read(IntMemoryLocation(IntBank::B, 1)));
  EXPECT_EQ(memory_->Read(IntMemoryLocation(IntBank::B, 8, 4)), 0b1001)
      << std::bitset<4>(memory_->Read(IntMemoryLocation(IntBank::B, 8, 4)));

  EXPECT_THROW(Write(4, 0, 0b10000), std::overflow_error);
  EXPECT_THROW(Write(5, 0, 0), std::invalid_argument);
}

TEST_F(MemoryTest, IntFill) {
  {
    memory_->Fill(IntBank::B, 50, 100, 7);

    for (size_t i = 50; i < 100; ++i) {
      IntMemoryLocation loc(IntBank::B, i);
      EXPECT_EQ(memory_->Read(loc), 7);
    }

    IntMemoryLocation loc_before(IntBank::B, 49);
    IntMemoryLocation loc_after(IntBank::B, 100);
    EXPECT_EQ(memory_->Read(loc_before), 0)
        << "Values outside the range should remain default";
    EXPECT_EQ(memory_->Read(loc_after), 0)
        << "Values outside the range should remain default";

    EXPECT_THROW(memory_->Fill(IntBank::B, 100, 50, 5), std::invalid_argument)
        << "Should throw when range (begin > end)";
    EXPECT_THROW(memory_->Fill(IntBank::B, 1990, 2010, 5), std::out_of_range);
  }

  {
    memory_->Fill(StrBank::M, 20, 30, "TestString");

    for (size_t i = 20; i < 30; ++i) {
      StrMemoryLocation loc(StrBank::M, i);
      EXPECT_EQ(memory_->Read(loc), "TestString");
    }

    StrMemoryLocation loc_before(StrBank::M, 19);
    StrMemoryLocation loc_after(StrBank::M, 30);
    EXPECT_EQ(memory_->Read(loc_before), "");
    EXPECT_EQ(memory_->Read(loc_after), "");

    EXPECT_THROW(memory_->Fill(StrBank::M, 30, 20, "Invalid"),
                 std::invalid_argument);
    EXPECT_THROW(memory_->Fill(StrBank::M, 1995, 2005, "OutOfRange"),
                 std::out_of_range);
  }
}

TEST_F(MemoryTest, Resize) {
  {
    IntMemoryLocation loc(IntBank::C, 2500), loc_existing(IntBank::C, 1999);
    memory_->Write(loc_existing, 77);
    memory_->Resize(IntBank::C, 3000);

    EXPECT_EQ(memory_->Read(loc_existing), 77)
        << "Existing data should be preserved";
    EXPECT_EQ(memory_->Read(loc), 0)
        << "New elements should be default initialized";

    // Resize to a smaller size
    memory_->Resize(IntBank::C, 1000);
    EXPECT_THROW(memory_->Read(loc_existing), std::out_of_range);
    memory_->Resize(IntBank::C, 8888);
    EXPECT_EQ(memory_->Read(loc), 0);
  }

  {
    StrMemoryLocation loc(StrBank::K, 2500), loc_existing(StrBank::K, 1999);
    memory_->Resize(StrBank::K, 3000);
    memory_->Write(loc_existing, "Hello, World!");

    EXPECT_EQ(memory_->Read(loc_existing), "Hello, World!");
    EXPECT_EQ(memory_->Read(loc), "");

    memory_->Resize(StrBank::K, 0);
    EXPECT_THROW(memory_->Read(loc_existing), std::out_of_range);
  }
}

TEST_F(MemoryTest, EdgeCases) {
  IntMemoryLocation invalid_loc(IntBank::D, static_cast<size_t>(-1));
  EXPECT_THROW(memory_->Read(invalid_loc), std::out_of_range);
  EXPECT_THROW(memory_->Write(invalid_loc, 10), std::out_of_range);

  EXPECT_THROW(memory_->Read(IntMemoryLocation(static_cast<IntBank>(100), 0)),
               std::invalid_argument)
      << "Should throw when an invalid memory bank is specified";

  IntMemoryLocation loc(IntBank::E, 0);
  memory_->Resize(IntBank::E, 0);
  EXPECT_THROW(memory_->Read(loc), std::out_of_range);
}

TEST_F(MemoryTest, GetStack) {
  for (int i = 0; i < 15; ++i) {
    memory_->Write(IntBank::L, i, i);
    memory_->Write(StrBank::K, i, std::to_string(i));
  }
  auto frame1 = memory_->StackMemory();

  for (int i = 10; i < 20; ++i) {
    memory_->Write(IntBank::L, i, i * i);
    memory_->Write(StrBank::K, i, std::to_string(i * i));
  }
  auto frame2 = memory_->StackMemory();

  // check stack frame 1
  for (int i = 0; i < 15; ++i) {
    EXPECT_EQ(frame1.L.Get(i), i);
    EXPECT_EQ(frame1.K.Get(i), std::to_string(i));
  }

  // check stack frame 2
  for (int i = 10; i < 20; ++i) {
    EXPECT_EQ(frame2.L.Get(i), i * i);
    EXPECT_EQ(frame2.K.Get(i), std::to_string(i * i));
  }

  // check memory
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(memory_->Read(IntBank::L, i), i);
    EXPECT_EQ(memory_->Read(StrBank::K, i), std::to_string(i));
  }
  for (int i = 10; i < 20; ++i) {
    EXPECT_EQ(memory_->Read(IntBank::L, i), i * i);
    EXPECT_EQ(memory_->Read(StrBank::K, i), std::to_string(i * i));
  }
}

TEST_F(MemoryTest, SetStack) {
  Memory::Stack frame1, frame2;
  frame1.L.Resize(50);
  frame1.K.Resize(50);
  frame2.L.Resize(60);
  frame2.K.Resize(60);

  for (int i = 0; i < 15; ++i) {
    frame1.L.Set(i, i);
    frame1.K.Set(i, std::to_string(i));
  }
  for (int i = 10; i < 20; ++i) {
    frame2.L.Set(i, i * i);
    frame2.K.Set(i, std::to_string(i * i));
  }

  memory_->Fill(IntBank::L, 0, 100, -123);
  memory_->Fill(StrBank::K, 0, 100, "some string");
  memory_->PartialReset(frame1);
  for (int i = 0; i < 15; ++i) {
    EXPECT_EQ(memory_->Read(IntBank::L, i), i);
    EXPECT_EQ(memory_->Read(StrBank::K, i), std::to_string(i));
  }

  memory_->PartialReset(frame2);
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(memory_->Read(IntBank::L, i), 0);
    EXPECT_EQ(memory_->Read(StrBank::K, i), "");
  }
  for (int i = 10; i < 20; ++i) {
    EXPECT_EQ(memory_->Read(IntBank::L, i), i * i);
    EXPECT_EQ(memory_->Read(StrBank::K, i), std::to_string(i * i));
  }
}

class MemoryStressTest : public ::testing::Test {
 protected:
  MemoryStressTest()
      : gen(rd()),
        size_dist(0, std::numeric_limits<size_t>::max()),
        int_dist(std::numeric_limits<int>::min(),
                 std::numeric_limits<int>::max()),
        bankid_dist(0, static_cast<uint8_t>(IntBank::CNT) - 1),
        memory_(std::make_shared<Memory>()) {}

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<size_t> size_dist;
  std::uniform_int_distribution<int> int_dist;
  std::uniform_int_distribution<uint8_t> bankid_dist;

  IntMemoryLocation RandomIntLocation() {
    return IntMemoryLocation(static_cast<IntBank>(bankid_dist(gen)),
                             size_dist(gen));
  }

  std::shared_ptr<Memory> memory_;
};

TEST_F(MemoryStressTest, DynamicAllocation) {
  auto fake_memory = [&]() -> std::map<IntMemoryLocation, int> {
    std::map<IntMemoryLocation, int> result;
    for (int i = 0; i < 1000; ++i)
      result.emplace(RandomIntLocation(), int_dist(gen));
    return result;
  }();

  EXPECT_NO_THROW({
    for (uint8_t i = 0; i < static_cast<uint8_t>(IntBank::CNT); ++i)
      memory_->Resize(static_cast<IntBank>(i),
                      std::numeric_limits<size_t>::max());
  }) << "Memory class should dynamically allocate memory on demand";

  for (const auto& [loc, val] : fake_memory)
    memory_->Write(loc, val);

  std::vector<std::map<IntMemoryLocation, int>> expect{fake_memory};
  std::vector<std::shared_ptr<Memory>> actual_version{
      std::make_shared<Memory>(*memory_)};

  // Create 10 versions of memory
  for (int ver = 1; ver < 10; ++ver) {
    for (int j = 0; j < 30; ++j) {
      auto loc = RandomIntLocation();
      auto val = int_dist(gen);
      memory_->Write(loc, val);
      fake_memory[loc] = val;
    }

    expect.push_back(fake_memory);
    actual_version.emplace_back(std::make_shared<Memory>(*memory_));
  }

  // Check the values
  for (int ver = 0; ver < 10; ++ver) {
    for (const auto& [loc, val] : expect[ver]) {
      EXPECT_EQ(actual_version[ver]->Read(loc), val)
          << "version " << ver << " location " << loc;
    }
  }
}

TEST_F(MemoryStressTest, CopyOnWrite) {
  Memory memory_copy = *memory_;

  IntMemoryLocation loc(IntBank::F, 500);
  memory_->Write(loc, 123);

  EXPECT_EQ(memory_copy.Read(loc), 0) << "The copy should not be affected";
  memory_copy.Write(loc, 456);

  EXPECT_EQ(memory_->Read(loc), 123)
      << "Original memory should not be affected";
  EXPECT_EQ(memory_copy.Read(loc), 456);
}
