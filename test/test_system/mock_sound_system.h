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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#ifndef TEST_TEST_SYSTEM_MOCK_SOUND_SYSTEM_H_
#define TEST_TEST_SYSTEM_MOCK_SOUND_SYSTEM_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "systems/base/sound_system.h"

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

#endif
