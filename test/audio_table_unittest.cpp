// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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
// -----------------------------------------------------------------------

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "core/audio_table.hpp"
#include "test_utils.hpp"

#include <sstream>
#include <string>

using std::string_literals::operator""s;

class AudioTableTest : public ::testing::Test {
 protected:
  void SetUp() override {
    gexe_ = Gameexe(LocateTestCase("Gameexe_data/Gameexe_soundsys.ini"));
    atable_ = AudioTable(gexe_);
  }

  Gameexe gexe_;
  AudioTable atable_;
};

TEST_F(AudioTableTest, CanParseSE) {
  const auto& se = atable_.se_table();

  EXPECT_EQ(se.at(0), std::make_pair(""s, 1));
  EXPECT_EQ(se.at(1), std::make_pair("se90"s, 0));
  EXPECT_EQ(se.at(2), std::make_pair("se91"s, 1));
  EXPECT_EQ(se.at(3), std::make_pair(""s, 0));
}

TEST_F(AudioTableTest, CanParseDS) {
  const auto& ds = atable_.ds_table();
  EXPECT_EQ(ds.at("bgm01"s), DSTrack("bgm01"s, "BGM01"s, 0, 2469380, 0));
  EXPECT_EQ(ds.at("bgm02"s), DSTrack("bgm02"s, "BGM02"s, 0, 2034018, 50728));
  EXPECT_EQ(ds.at("bgm03"s), DSTrack("bgm03"s, "BGM03"s, 0, 3127424, 1804));
}

TEST_F(AudioTableTest, CanParseCD) {
  const auto& cd = atable_.cd_table();
  EXPECT_EQ(cd.at("cdbgm04"s), CDTrack("cdbgm04"s, 0, 6093704, 3368845));
}

TEST_F(AudioTableTest, CanParseBGM) {
  const auto& ds = atable_.ds_table();
  EXPECT_EQ(ds.at("bgm05"s),
            DSTrack("bgm05"s, "BGM01"s, 82286, 5184000, 905143));
  EXPECT_EQ(ds.at("bgm06"s),
            DSTrack("bgm06"s, "BGM02"s, 147692, 7015385, 221538));
}
