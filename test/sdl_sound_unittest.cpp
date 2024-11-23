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
// -----------------------------------------------------------------------

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test_utils.hpp"

#include "base/avspec.hpp"
#include "systems/sdl/sdl_sound_system.hpp"
#include "systems/sdl/sound_implementor.hpp"

#include <filesystem>
#include <memory>
#include <tuple>

class FakeAudioImpl : public SDLSoundImpl {
 public:
  using super = SDLSoundImpl;

  FakeAudioImpl() = default;
  ~FakeAudioImpl() = default;

  using super::FromSDLSoundFormat;
  using super::ToSDLSoundFormat;
};
using ::testing::EndsWith;
using ::testing::Return;

namespace fs = std::filesystem;

TEST(SDLSound, SoundFormat) {
  // sdl1.2 audio format flags
  constexpr auto AUDIO_S8 = 0x8008;
  constexpr auto AUDIO_U8 = 0x0008;
  constexpr auto AUDIO_S16SYS = 0x8010;

  auto aimpl = std::make_shared<FakeAudioImpl>();
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::U8), AUDIO_U8);
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S8), AUDIO_S8);
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S16), AUDIO_S16SYS);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S64),
               std::invalid_argument);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::DBL),
               std::invalid_argument);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::NONE),
               std::invalid_argument);

  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_U8), AV_SAMPLE_FMT::U8);
  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_S8), AV_SAMPLE_FMT::S8);
  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_S16SYS), AV_SAMPLE_FMT::S16);

  EXPECT_THROW(aimpl->FromSDLSoundFormat(0), std::invalid_argument);
  EXPECT_THROW(aimpl->FromSDLSoundFormat(12345), std::invalid_argument);
}
