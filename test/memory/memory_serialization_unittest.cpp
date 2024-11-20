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

#include "memory/memory.hpp"
#include "memory/serialization_global.hpp"
#include "memory/serialization_local.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <sstream>
#include <vector>

class MemorySerializationTest : public ::testing::Test {
 protected:
  std::vector<IntBank> IntBanks(std::string s) {
    std::vector<IntBank> result;
    for (char c : s)
      result.push_back(ToIntBank(c));
    return result;
  }
};

TEST_F(MemorySerializationTest, Global) {
  std::stringstream ss;

  {
    Memory memory;
    memory.Write(IntBank::G, 0, 10);
    memory.Write(IntBank::Z, 1, 11);
    memory.Write(StrBank::M, 2, "12");
    memory.Write(StrBank::global_name, 3, "Furukawa");
    memory.Write(StrBank::global_name, 4, "Nagisa");

    boost::archive::text_oarchive oa(ss);
    oa << memory.GetGlobalMemory();
  }

  {
    GlobalMemory deserialized;
    boost::archive::text_iarchive ia(ss);
    ia >> deserialized;

    EXPECT_EQ(deserialized.G.Get(0), 10);
    EXPECT_EQ(deserialized.Z.Get(1), 11);
    EXPECT_EQ(deserialized.M.Get(2), "12");
    EXPECT_EQ(deserialized.global_names.Get(4) + ' ' +
                  deserialized.global_names.Get(3),
              "Nagisa Furukawa");
  }
}

TEST_F(MemorySerializationTest, Local) {
  std::stringstream ss;

  {
    Memory memory;
    int value = 0;
    for (auto bank : IntBanks("ABCDEFXHIJ")) {
      for (int i = 0; i < 100; ++i)
        memory.Write(bank, i, value++);
    }
    for (int i = 0; i < 100; ++i)
      memory.Write(StrBank::S, i, std::to_string(value++));
    for (int i = 0; i < 100; ++i)
      memory.Write(StrBank::local_name, i, std::to_string(value++));

    boost::archive::text_oarchive oa(ss);
    oa << memory.GetLocalMemory();
  }

  {
    LocalMemory deserialized;
    boost::archive::text_iarchive ia(ss);
    ia >> deserialized;

    int expect_value = 0;
    for (auto bank : std::vector<MemoryBank<int>>{
             deserialized.A, deserialized.B, deserialized.C, deserialized.D,
             deserialized.E, deserialized.F, deserialized.X, deserialized.H,
             deserialized.I, deserialized.J}) {
      for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(bank.Get(i), expect_value);
        ++expect_value;
      }
    }

    for (int i = 0; i < 100; ++i) {
      EXPECT_EQ(deserialized.S.Get(i), std::to_string(expect_value));
      ++expect_value;
    }
    for (int i = 0; i < 100; ++i) {
      EXPECT_EQ(deserialized.local_names.Get(i), std::to_string(expect_value));
      ++expect_value;
    }
  }
}
