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

#include "base/kidoku_table.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <sstream>

class KidokuTableTest : public ::testing::Test {
 protected:
  KidokuTable kidoku_table;
};

TEST_F(KidokuTableTest, InitialState) {
  EXPECT_FALSE(kidoku_table.HasBeenRead(1, 0));
  EXPECT_FALSE(kidoku_table.HasBeenRead(0, 0));
  EXPECT_FALSE(kidoku_table.HasBeenRead(-1, 0));
  EXPECT_FALSE(kidoku_table.HasBeenRead(1, 100));
}

TEST_F(KidokuTableTest, RecordAndCheck) {
  int scenario = 1;
  int kidoku = 0;

  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario, kidoku));
  kidoku_table.RecordKidoku(scenario, kidoku);
  EXPECT_TRUE(kidoku_table.HasBeenRead(scenario, kidoku));
  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario, kidoku + 1));
}

TEST_F(KidokuTableTest, MultipleScenarios) {
  int scenario1 = 1;
  int kidoku1 = 0;
  int scenario2 = 2;
  int kidoku2 = 1;

  kidoku_table.RecordKidoku(scenario1, kidoku1);
  kidoku_table.RecordKidoku(scenario2, kidoku2);
  EXPECT_TRUE(kidoku_table.HasBeenRead(scenario1, kidoku1));
  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario1, kidoku1 + 1));

  EXPECT_TRUE(kidoku_table.HasBeenRead(scenario2, kidoku2));
  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario2, kidoku2 - 1));
}

TEST_F(KidokuTableTest, ResizingBehavior) {
  int scenario = 1;
  int high_kidoku = 1000;

  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario, high_kidoku));
  kidoku_table.RecordKidoku(scenario, high_kidoku);
  EXPECT_TRUE(kidoku_table.HasBeenRead(scenario, high_kidoku));
  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario, high_kidoku - 1));
}

TEST_F(KidokuTableTest, RepeatedRecording) {
  int scenario = 1;
  int kidoku = 5;

  kidoku_table.RecordKidoku(scenario, kidoku);
  kidoku_table.RecordKidoku(scenario, kidoku);
  kidoku_table.RecordKidoku(scenario, kidoku);
  EXPECT_TRUE(kidoku_table.HasBeenRead(scenario, kidoku));
}

TEST_F(KidokuTableTest, MultipleKidokus) {
  int scenario = 1;
  std::vector<int> kidokus = {0, 1, 2, 3, 4, 5};

  for (int kidoku : kidokus) {
    kidoku_table.RecordKidoku(scenario, kidoku);
  }

  for (int kidoku : kidokus) {
    EXPECT_TRUE(kidoku_table.HasBeenRead(scenario, kidoku));
  }

  EXPECT_FALSE(kidoku_table.HasBeenRead(scenario, 6));
}

TEST_F(KidokuTableTest, Serialization) {
  const int scenario_nums = 100;
  const int kidoku_nums = 100;

  std::stringstream ss;
  {
    KidokuTable table;
    for (int i = 1; i < scenario_nums; ++i)
      for (int j = 0; j < kidoku_nums; ++j)
        table.RecordKidoku(i, j * i);

    boost::archive::text_oarchive oa(ss);
    oa << table;
  }

  {
    boost::archive::text_iarchive ia(ss);
    ia >> kidoku_table;

    for (int i = 1; i < scenario_nums; ++i)
      for (int j = 0; j < i * kidoku_nums; ++j) {
        EXPECT_EQ(kidoku_table.HasBeenRead(i, j), j % i == 0);
      }
  }
}
