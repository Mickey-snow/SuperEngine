// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "test_utils.h"

#include "libreallive/gameexe.h"

#include <sstream>
#include <string>

using std::string_literals::operator""s;

class MockEventSystem : public EventSystem {
 public:
  MockEventSystem(Gameexe& gexe) : EventSystem(gexe) {}

  MOCK_METHOD(void, ExecuteEventSystem, (RLMachine & machine), (override));
  MOCK_METHOD(unsigned int, GetTicks, (), (const, override));
  MOCK_METHOD(void, Wait, (unsigned int milliseconds), (const, override));
  MOCK_METHOD(bool, ShiftPressed, (), (const, override));
  MOCK_METHOD(bool, CtrlPressed, (), (const, override));
  MOCK_METHOD(Point, GetCursorPos, (), (override));
  MOCK_METHOD(void,
              GetCursorPos,
              (Point & position, int& button1, int& button2),
              (override));
  MOCK_METHOD(void, FlushMouseClicks, (), (override));
  MOCK_METHOD(unsigned int, TimeOfLastMouseMove, (), (override));
  MOCK_METHOD(void,
              InjectMouseMovement,
              (RLMachine & machine, const Point& loc),
              (override));

  MOCK_METHOD(void, InjectMouseDown, (RLMachine & machine), (override));
  MOCK_METHOD(void, InjectMouseUp, (RLMachine & machine), (override));
};

class MockSystem : public System {
 public:
  MockSystem(Gameexe& gexe) : gexe_(gexe) {}

  MOCK_METHOD(void, Run, (RLMachine&), (override));
  MOCK_METHOD(GraphicsSystem&, graphics, (), (override));
  MOCK_METHOD(EventSystem&, event, (), (override));
  MOCK_METHOD(TextSystem&, text, (), (override));
  MOCK_METHOD(SoundSystem&, sound, (), (override));

  Gameexe& gameexe() override { return gexe_; }

  Gameexe& gexe_;
};

class MockSoundSystem : public SoundSystem {
 public:
  MockSoundSystem(System& sys) : SoundSystem(sys) {}

  MOCK_METHOD(int, BgmStatus, (), (const, override));
  MOCK_METHOD(void, BgmPlay, (const std::string&, bool), (override));
  MOCK_METHOD(void, BgmPlay, (const std::string&, bool, int), (override));
  MOCK_METHOD(void, BgmPlay, (const std::string&, bool, int, int), (override));
  MOCK_METHOD(void, BgmStop, (), (override));
  MOCK_METHOD(void, BgmPause, (), (override));
  MOCK_METHOD(void, BgmUnPause, (), (override));
  MOCK_METHOD(void, BgmFadeOut, (int), (override));
  MOCK_METHOD(std::string, GetBgmName, (), (const, override));
  MOCK_METHOD(bool, BgmLooping, (), (const, override));
  MOCK_METHOD(void, WavPlay, (const std::string&, bool), (override));
  MOCK_METHOD(void, WavPlay, (const std::string&, bool, int), (override));
  MOCK_METHOD(void, WavPlay, (const std::string&, bool, int, int), (override));
  MOCK_METHOD(bool, WavPlaying, (const int), (override));
  MOCK_METHOD(void, WavStop, (const int), (override));
  MOCK_METHOD(void, WavStopAll, (), (override));
  MOCK_METHOD(void, WavFadeOut, (const int, const int), (override));
  MOCK_METHOD(void, PlaySe, (const int), (override));
  MOCK_METHOD(bool, KoePlaying, (), (const, override));
  MOCK_METHOD(void, KoeStop, (), (override));
  MOCK_METHOD(void, KoePlayImpl, (int), (override));

  // methods exposed for testing
  using SoundSystem::cd_table;
  using SoundSystem::ds_table;
  using SoundSystem::se_table;
};

using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnRef;

class SoundSystemTest : public ::testing::Test {
 protected:
  SoundSystemTest()
      : gexe_(LocateTestCase("Gameexe_data/Gameexe_soundsys.ini")),
        msys_(gexe_),
        mevent_sys_(gexe_),
        msound_sys_(msys_) {}

  void SetUp() override {
    ON_CALL(msys_, event()).WillByDefault(ReturnRef(mevent_sys_));
  }

  Gameexe gexe_;
  MockSystem msys_;
  MockEventSystem mevent_sys_;
  MockSoundSystem msound_sys_;
};

// Makes sure we can parse the bizarre Gameexe.ini keys for KOEONOFF
TEST_F(SoundSystemTest, CanParseKOEONOFFKeys) {
  SoundSystem& sys = msound_sys_;

  // Test the UseKoe side of things
  EXPECT_EQ(1, sys.ShouldUseKoeForCharacter(0));
  EXPECT_EQ(0, sys.ShouldUseKoeForCharacter(7));
  EXPECT_EQ(1, sys.ShouldUseKoeForCharacter(8));

  // Test the koePlay side of things
  EXPECT_EQ(5, sys.globals().character_koe_enabled.size());
  EXPECT_EQ(1, sys.globals().character_koe_enabled[0]);
  EXPECT_EQ(0, sys.globals().character_koe_enabled[3]);
  EXPECT_EQ(1, sys.globals().character_koe_enabled[2]);
  EXPECT_EQ(1, sys.globals().character_koe_enabled[20]);
  EXPECT_EQ(1, sys.globals().character_koe_enabled[105]);
}

// Tests that SetUseKoe sets values correctly
TEST_F(SoundSystemTest, SetUseKoeCorrectly) {
  SoundSystem& sys = msound_sys_;

  sys.SetUseKoeForCharacter(0, 0);
  sys.SetUseKoeForCharacter(7, 1);
  sys.SetUseKoeForCharacter(8, 0);

  // Make sure all values are flipped from the previous test.

  // Test the UseKoe side of things
  EXPECT_EQ(0, sys.ShouldUseKoeForCharacter(0));
  EXPECT_EQ(1, sys.ShouldUseKoeForCharacter(7));
  EXPECT_EQ(0, sys.ShouldUseKoeForCharacter(8));

  // Test the koePlay side of things
  EXPECT_EQ(5, sys.globals().character_koe_enabled.size());
  EXPECT_EQ(0, sys.globals().character_koe_enabled[0]);
  EXPECT_EQ(1, sys.globals().character_koe_enabled[3]);
  EXPECT_EQ(0, sys.globals().character_koe_enabled[2]);
  EXPECT_EQ(0, sys.globals().character_koe_enabled[20]);
  EXPECT_EQ(0, sys.globals().character_koe_enabled[105]);
}

// Make sure we thaw previously serialized character_koe_enabled data correctly.
TEST_F(SoundSystemTest, SetUseKoeSerialization) {
  std::stringstream ss;
  {
    SoundSystem& sys = msound_sys_;

    // Reverse the values as in <2>.
    sys.SetUseKoeForCharacter(0, 0);
    sys.SetUseKoeForCharacter(7, 1);
    sys.SetUseKoeForCharacter(8, 0);

    boost::archive::text_oarchive oa(ss);
    oa << const_cast<const SoundSystemGlobals&>(sys.globals());
  }
  {
    Gameexe mygexe = gexe_;
    MockSystem mySystem(mygexe);
    MockSoundSystem mySoundSystem(mySystem);

    SoundSystem& sys = mySoundSystem;
    boost::archive::text_iarchive ia(ss);
    ia >> sys.globals();

    // Do the flip tests as in <2>

    // Test the UseKoe side of things
    EXPECT_EQ(0, sys.ShouldUseKoeForCharacter(0));
    EXPECT_EQ(1, sys.ShouldUseKoeForCharacter(7));
    EXPECT_EQ(0, sys.ShouldUseKoeForCharacter(8));

    // Test the koePlay side of things
    EXPECT_EQ(5, sys.globals().character_koe_enabled.size());
    EXPECT_EQ(0, sys.globals().character_koe_enabled[0]);
    EXPECT_EQ(1, sys.globals().character_koe_enabled[3]);
    EXPECT_EQ(0, sys.globals().character_koe_enabled[2]);
    EXPECT_EQ(0, sys.globals().character_koe_enabled[20]);
    EXPECT_EQ(0, sys.globals().character_koe_enabled[105]);
  }
}

TEST_F(SoundSystemTest, CanParseSEDSCD) {
  {
    const auto& se = msound_sys_.se_table();

    EXPECT_EQ(se.at(0), std::make_pair(""s, 1));
    EXPECT_EQ(se.at(1), std::make_pair("se90"s, 0));
    EXPECT_EQ(se.at(2), std::make_pair("se91"s, 1));
    EXPECT_EQ(se.at(3), std::make_pair(""s, 0));
  }

  {
    const auto& ds = msound_sys_.ds_table();

    using DSTrack = SoundSystem::DSTrack;
    EXPECT_EQ(ds.at("bgm01"s), DSTrack("bgm01"s, "BGM01"s, 0, 2469380, 0));
    EXPECT_EQ(ds.at("bgm02"s), DSTrack("bgm02"s, "BGM02"s, 0, 2034018, 50728));
    EXPECT_EQ(ds.at("bgm03"s), DSTrack("bgm03"s, "BGM03"s, 0, 3127424, 1804));
  }

  {
    const auto& cd = msound_sys_.cd_table();

    using CDTrack = SoundSystem::CDTrack;
    EXPECT_EQ(cd.at("cdbgm04"s), CDTrack("cdbgm04"s, 0, 6093704, 3368845));
  }
}
