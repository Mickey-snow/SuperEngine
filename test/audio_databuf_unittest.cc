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

#include "base/audio_data.h"

TEST(SampleConversionTest, S16ToFloatConversion) {
  std::vector<avsample_s16_t> s16_audio{32767, -32768, 0, -128};
  AudioData audio_data;
  audio_data.data = s16_audio;
  std::vector<avsample_flt_t> result = audio_data.GetAs<avsample_flt_t>();

  EXPECT_FLOAT_EQ(result[0], 1.0f) << "Max signed 16-bit should map to 1.0";
  EXPECT_FLOAT_EQ(result[1], -1.0f) << "Min signed 16-bit should map to -1.0";
  EXPECT_FLOAT_EQ(result[2], 0.0f) << "Zero should map to 0.0";
  EXPECT_FLOAT_EQ(result[3], -3.90625e-3);
}

TEST(SampleConversionTest, FloatToS16) {
  std::vector<avsample_flt_t> flt_audio{1.0f, -1.0f, 0.0f, .3f, -.5f};
  AudioData audio_data;
  audio_data.data = flt_audio;
  std::vector<avsample_s16_t> result = audio_data.GetAs<avsample_s16_t>();

  EXPECT_EQ(result[0], 32767) << "Max float should map to max signed 16-bit";
  EXPECT_EQ(result[1], -32768) << "Min float should map to min signed 16-bit";
  EXPECT_EQ(result[2], 0);
  EXPECT_EQ(result[3], 9830);
  EXPECT_EQ(result[4], -16384);
}

TEST(SampleConversionTest, U8ToS16) {
  std::vector<avsample_u8_t> u8_audio{255, 0, 96, 95};
  AudioData audio_data;
  audio_data.data = u8_audio;
  std::vector<avsample_s16_t> result = audio_data.GetAs<avsample_s16_t>();

  EXPECT_EQ(result[0], 32767)
      << "Max unsigned 8-bit should map to max signed 16-bit";
  EXPECT_EQ(result[1], -32768)
      << "Min unsigned 8-bit should map to min signed 16-bit";
  EXPECT_EQ(result[2], -8095);
  EXPECT_EQ(result[3], -8352);
}

TEST(SampleConversionTest, S8ToS16) {
  std::vector<avsample_s8_t> s8_audio{127, -128, 0, 64, -64};
  AudioData audio_data;
  audio_data.data = s8_audio;
  std::vector<avsample_s16_t> result = audio_data.GetAs<avsample_s16_t>();

  EXPECT_EQ(result[0], 32767)
      << "Max signed 8-bit should map to max signed 16-bit";
  EXPECT_EQ(result[1], -32768)
      << "Min signed 8-bit should map to min signed 16-bit";
  EXPECT_EQ(result[2], 0);
  EXPECT_EQ(result[3], 16512);
  EXPECT_EQ(result[4], -16384);
}

// Test conversion from S32 to Float
TEST(SampleConversionTest, S32ToFloat) {
  std::vector<avsample_s32_t> s32_audio{2147483647, -2147483648, 0, 536870912};
  AudioData audio_data;
  audio_data.data = s32_audio;
  std::vector<avsample_flt_t> result = audio_data.GetAs<avsample_flt_t>();

  EXPECT_FLOAT_EQ(result[0], 1.0f)
      << "Max signed 32-bit should map to 1.0 float";
  EXPECT_FLOAT_EQ(result[1], -1.0f)
      << "Min signed 32-bit should map to -1.0 float";
  EXPECT_FLOAT_EQ(result[2], 0.0f);
  EXPECT_FLOAT_EQ(result[3], 0.25f);
}

// Test conversion from Float to S32
TEST(SampleConversionTest, FloatToS32) {
  std::vector<avsample_flt_t> flt_audio{1.0f, -1.0f, 0.0f, 0.5f, -0.5f};
  AudioData audio_data;
  audio_data.data = flt_audio;
  std::vector<avsample_s32_t> result = audio_data.GetAs<avsample_s32_t>();

  EXPECT_EQ(result[0], 2147483647)
      << "Max float should map to max signed 32-bit";
  EXPECT_EQ(result[1], -2147483648)
      << "Min float should map to min signed 32-bit";
  EXPECT_EQ(result[2], 0);
  EXPECT_EQ(result[3], 1073741823);
  EXPECT_EQ(result[4], -1073741824);
}

TEST(SampleConversionTest, S64ToDouble) {
  std::vector<avsample_s64_t> s64_audio{9223372036854775807LL,
                                        -9223372036854775807LL, 0LL,
                                        4611686018427387904LL};
  AudioData audio_data;
  audio_data.data = s64_audio;
  std::vector<avsample_dbl_t> result = audio_data.GetAs<avsample_dbl_t>();

  EXPECT_DOUBLE_EQ(result[0], 1.0)
      << "Max signed 64-bit should map to 1.0 double";
  EXPECT_DOUBLE_EQ(result[1], -1.0)
      << "Min signed 64-bit should map to -1.0 double";
  EXPECT_DOUBLE_EQ(result[2], 0.0);
  EXPECT_DOUBLE_EQ(result[3], 0.5);
}

TEST(SampleConversionTest, U8ToFloat) {
  std::vector<avsample_u8_t> u8_audio{255, 0, 128, 64};
  AudioData audio_data;
  audio_data.data = u8_audio;
  std::vector<avsample_flt_t> result = audio_data.GetAs<avsample_flt_t>();

  EXPECT_FLOAT_EQ(result[0], 1.0f)
      << "Max unsigned 8-bit should map to 1.0 float";
  EXPECT_FLOAT_EQ(result[1], -1.0f)
      << "Min unsigned 8-bit should map to -1.0 float";
  EXPECT_NEAR(result[2], 0.0f, 1.0f / 255);
  EXPECT_NEAR(result[3], -0.5f, 1.0f / 255);
}

TEST(AudioDataTest, SampleLength) {
  {
    std::vector<avsample_u8_t> u8_audio{255, 0, 128, 64};
    AudioData audio_data;
    audio_data.data = u8_audio;
    audio_data.spec = {.sample_rate = 44100,
                       .sample_format = AV_SAMPLE_FMT::U8,
                       .channel_count = 1};
    EXPECT_EQ(audio_data.ByteLength(), 4);
  }

  {
    std::vector<avsample_s16_t> s16_audio{32767, -32768, 0, -128, 33};
    AudioData audio_data;
    audio_data.data = s16_audio;
    audio_data.spec = {.sample_rate = 44100,
                       .sample_format = AV_SAMPLE_FMT::S16,
                       .channel_count = 1};
    EXPECT_EQ(audio_data.ByteLength(), 10);
  }
}
