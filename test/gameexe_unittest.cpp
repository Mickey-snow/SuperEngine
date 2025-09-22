// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
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

#include "core/gameexe.hpp"

#include "test_utils.hpp"

#include <string>
#include <vector>

TEST(GameexeUnit, ReadAllKeys) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  EXPECT_EQ(26, ini.Size()) << "Wrong number of keys";
}

TEST(GameexeUnit, SiglusFormatParsing) {
  Gameexe ini(LocateTestCase("Gameexe_data/siglus.ini"));

  ASSERT_TRUE(ini("BGM.000").Exists());
  EXPECT_EQ((std::vector<std::string>{"BGM01", "BGM01", "82286", "5184000",
                                      "905143"}),
            ini("BGM.000").ToStrVector());

  EXPECT_EQ(string("Noto Serif JP Medium"), ini("CONFIG.FONT.NAME").ToString());
  EXPECT_EQ(1, ini("CONFIG.SWITCH.ON").ToInt());

  auto chrEntry = ini("CHR.ENTRY").ToStrVector();
  ASSERT_EQ(4u, chrEntry.size());
  EXPECT_EQ("Hero", chrEntry.at(0));
  EXPECT_TRUE(chrEntry.at(1).empty());
  EXPECT_EQ(1, ini("CHR.ENTRY").GetIntAt(2));
  EXPECT_EQ(255, ini("CHR.ENTRY").GetIntAt(3));

  EXPECT_EQ(-1, ini("FLAGS").GetIntAt(0));
  EXPECT_EQ(0, ini("FLAGS").GetIntAt(1));
  EXPECT_EQ(1, ini("FLAGS").GetIntAt(2));

  auto textLine = ini("TEXT.LINE").ToStrVector();
  ASSERT_EQ(2u, textLine.size());
  EXPECT_EQ("Quoted value", textLine.at(0));
  EXPECT_EQ("next", textLine.at(1));
}

// Make sure #CAPTION exists and that we read its value correctly.
TEST(GameexeUnit, ReadsCaption) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  ASSERT_TRUE(ini("CAPTION").Exists()) << "#CAPTION should exist";
  EXPECT_EQ(string("Canon: A firearm"), ini("CAPTION").ToString())
      << "Wrong value for CAPTION";
}

// Make sure #RANDOM_KEY doesn't exist.
TEST(GameexeUnit, RandomKeyDoesntExist) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  EXPECT_FALSE(ini("RANDOM_KEY").Exists()) << "#RANDOM_KEY should not exist";
}

// Test ToIntVector() parsing.
TEST(GameexeUnit, IntVectorParsing) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  ASSERT_TRUE(ini("WINDOW_ATTR").Exists()) << "#WINDOW_ATTR should exist!";

  std::vector<int> ints = ini("WINDOW_ATTR").ToIntVector();
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(i + 1, ints.at(i));
  }
}

// Make sure operator() works with multiple keys...
TEST(GameexeUnit, MultipleKeys) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  EXPECT_EQ(1, ini("IMAGINE", "ONE"));
  EXPECT_EQ(2, ini("IMAGINE", "TWO"));
  EXPECT_EQ(3, ini("IMAGINE", "THREE"));

  ini("IMAGINE", "ONE") = 4;
  ini("IMAGINE", "TWO") = 5;
  ini("IMAGINE", "THREE") = 6;
  EXPECT_EQ(4, ini("IMAGINE", "ONE"));
  EXPECT_EQ(5, ini("IMAGINE", "TWO"));
  EXPECT_EQ(6, ini("IMAGINE", "THREE"));
}

// Make sure GameexeInterpretObject chaining works correctly.
TEST(GameexeUnit, ChainingWorks) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  auto imagine = ini("IMAGINE");
  EXPECT_EQ(1, imagine("ONE"));
  EXPECT_EQ(2, imagine("TWO"));
  EXPECT_EQ(3, imagine("THREE"));

  imagine("ONE") = -100;
  imagine("ONE") = 7;
  imagine("TWO") = 8;
  imagine("THREE") = 9;
  imagine("FOUR") = 10;
  EXPECT_EQ(7, ini("IMAGINE", "ONE"));
  EXPECT_EQ(8, ini("IMAGINE", "TWO"));
  EXPECT_EQ(9, ini("IMAGINE", "THREE"));
  EXPECT_EQ(10, ini("IMAGINE", "FOUR"));
  EXPECT_EQ(7, imagine("ONE"));
  EXPECT_EQ(8, imagine("TWO"));
  EXPECT_EQ(9, imagine("THREE"));
  EXPECT_EQ(10, imagine("FOUR"));
}

TEST(GameexeUnit, FilterRange) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));

  {
    std::vector<int> expected{1, 3, 2};
    std::vector<int> actual;
    for (auto it : ini.Filter("IMAGINE."))
      actual.push_back(it);
    EXPECT_EQ(expected, actual);
  }

  {
    std::multiset<std::string> expected{"-1", "0", "0",  "0",  "0",  "0",
                                        "1",  "2", "22", "25", "42", "90"};
    std::multiset<std::string> actual;
    for (const auto it : ini.Filter("WINDOW."))
      actual.insert(it);
    EXPECT_EQ(expected, actual);
  }
}

TEST(GameexeUnit, MultipleIterate) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));

  const std::vector<int> expected{1, 3, 2};
  auto filter_range = ini.Filter("IMAGINE");
  for (int i = 0; i < 10; ++i) {
    std::vector<int> actual;
    for (auto it : filter_range)
      actual.push_back(it);
    EXPECT_EQ(expected, actual);
  }
}

TEST(GameexeUnit, FilterEmpty) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  for (const auto it : ini.Filter("nonexist.OBJECT"))
    FAIL() << "Filter should be empty";
}

TEST(GameexeUnit, KeyParts) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe.ini"));
  auto gio = ini("WINDOW.000.ATTR_MOD");
  std::vector<string> pieces = gio.GetKeyParts();
  EXPECT_EQ(3, pieces.size());
  EXPECT_EQ("WINDOW", pieces[0]);
  EXPECT_EQ("000", pieces[1]);
  EXPECT_EQ("ATTR_MOD", pieces[2]);
}

// A demo on the Princess Bride Box Set does something weird with its \#DSTRACK
// keys. While all other games space the entries like this:
//
//   #DSTRACK = 00000000 - 99999999 - 00269364 = "BGM01"  = "BGM01"
//
// this game does not:
//
//   #DSTRACK=00000000-10998934-00000000="dcbgm000"="dcbgm000"
//
// and this runs afoul of the gameexe tokenization
// code. (00000000-10998934-00000000 is treated as a single token.)
//
// This test ensures that the gameexe parser can handle both.
TEST(GameexeUnit, DstrackRegression) {
  Gameexe ini(LocateTestCase("Gameexe_data/Gameexe_tokenization.ini"));

  GameexeInterpretObject clannad = ini("CLANNADDSTRACK");
  EXPECT_EQ(0, clannad.GetIntAt(0));
  EXPECT_EQ(99999999, clannad.GetIntAt(1));
  EXPECT_EQ(269364, clannad.GetIntAt(2));
  EXPECT_EQ("BGM01", clannad.GetStringAt(3));
  EXPECT_EQ("BGM01", clannad.GetStringAt(4));

  GameexeInterpretObject dc = ini("DCDSTRACK");
  EXPECT_EQ(0, dc.GetIntAt(0));
  EXPECT_EQ(10998934, dc.GetIntAt(1));
  EXPECT_EQ(0, dc.GetIntAt(2));
  EXPECT_EQ("dcbgm000", dc.GetStringAt(3));
  EXPECT_EQ("dcbgm000", dc.GetStringAt(4));
}
