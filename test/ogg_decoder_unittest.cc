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

#include "test_utils.h"

#include "base/avdec/ogg.h"
#include "base/avdec/wav.h"
#include "systems/base/ovk_voice_sample.h"
#include "utilities/mapped_file.h"
#include "utilities/numbers.h"

#include <cmath>
#include <fstream>
#include <iostream>

class OggDecoderTest : public ::testing::Test {
 protected:
  void SetUp() {
    sample_count = std::round(duration * sample_rate);
    file_str = LocateTestCase("Gameroot/OGG/test.ogg");
  }

  AVSpec DetermineSpecification() const {
    return {.sample_rate = sample_rate,
            .sample_format = AV_SAMPLE_FMT::S16,
            .channel_count = 2};
  }

  std::vector<double> ReproduceAudio() const {
    std::vector<double> result;
    for (int i = 0; i < sample_count; ++i) {
      float t = static_cast<float>(i) / sample_rate;
      result.push_back(std::sin(2 * pi * freq_left * t));
      result.push_back(std::sin(2 * pi * freq_right * t));
    }
    return result;
  }

  std::vector<double> Normalize(const avsample_buffer_t& a) const {
    return std::visit(
        [](auto& a) -> std::vector<double> {
          std::vector<double> result;
          for (const auto& it : a) {
            result.push_back(static_cast<double>(it) / 32767.0);
          }
          return result;
        },
        a);
  }

  double Deviation(const std::vector<double>& a,
                   const std::vector<double>& b) const {
    auto sqr = [](auto x) { return x * x; };
    size_t n = a.size();
    double var = 0;
    for (size_t i = 0; i < n; ++i)
      var += sqr(a[i] - b[i]) / n;
    return sqrt(var);
  }

  std::string file_str;
  const float duration = 0.2;
  const int sample_rate = 44100;
  const int freq_left = 440, freq_right = 554;
  int sample_count;
};

TEST_F(OggDecoderTest, OVKVoiceSample) {
  AudioData audio;

  {
    int len = 0;
    char* result = nullptr;
    OVKVoiceSample decoder(file_str);
    result = decoder.Decode(&len);

    audio = WavDecoder(std::string_view(result, len)).DecodeAll();
  }

  EXPECT_EQ(audio.spec, DetermineSpecification());
  EXPECT_LE(Deviation(Normalize(audio.data), ReproduceAudio()), 0.01);
}

TEST_F(OggDecoderTest, oggDecoder) {
  static constexpr double max_std = 0.01;

  auto file = MappedFile(file_str);
  OggDecoder decoder(file.Read());
  AudioData audio = decoder.DecodeAll();
  EXPECT_EQ(audio.spec, DetermineSpecification());

  auto actual_wav = Normalize(audio.data);
  auto expect_wav = ReproduceAudio();
  ASSERT_EQ(actual_wav.size(), expect_wav.size());
  EXPECT_LE(Deviation(expect_wav, actual_wav), max_std);
}
