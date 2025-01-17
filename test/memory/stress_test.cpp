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

#include <memory>
#include <random>

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
