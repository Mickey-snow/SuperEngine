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
#include "utilities/mapped_file.h"
#include "utilities/numbers.h"

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

class OggDecoderTest : public ::testing::Test {
 protected:
  void SetUp() {
    sample_count = std::round(duration * sample_rate);
    file_path = PathToTestCase("Gameroot/OGG/test.ogg");
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
    size_t n = std::min(a.size(), b.size());
    double var = 0;
    for (size_t i = 0; i < n; ++i)
      var += sqr(a[i] - b[i]) / n;
    return sqrt(var);
  }

  fs::path file_path;
  const float duration = 0.2;
  const int sample_rate = 44100;
  const int freq_left = 440, freq_right = 554;
  int sample_count;
};

TEST_F(OggDecoderTest, DecodeOgg) {
  static constexpr double max_std = 0.01;

  auto file = MappedFile(file_path);
  OggDecoder decoder(file.Read());
  AudioData audio = decoder.DecodeAll();
  EXPECT_EQ(audio.spec, DetermineSpecification());

  auto actual_wav = Normalize(audio.data);
  auto expect_wav = ReproduceAudio();
  ASSERT_EQ(actual_wav.size(), expect_wav.size());
  EXPECT_LE(Deviation(expect_wav, actual_wav), max_std);
}

TEST_F(OggDecoderTest, Rewind) {
  static constexpr double max_std = 0.01;

  auto file = MappedFile(file_path);
  OggDecoder decoder(file.Read());

  auto expect_wav = ReproduceAudio();
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(decoder.Seek(0, SEEKDIR::BEG), SEEK_RESULT::PRECISE_SEEK);
    AudioData result_front;
    {
      auto a = decoder.DecodeNext();
      auto b = decoder.DecodeNext();
      AudioData result_front = AudioData::Concat(std::move(a), std::move(b));
    }
    EXPECT_LE(Deviation(expect_wav, Normalize(result_front.data)), max_std);
    EXPECT_TRUE(decoder.HasNext());

    EXPECT_EQ(decoder.Seek(0, SEEKDIR::BEG), SEEK_RESULT::PRECISE_SEEK);
    auto actual_wav = Normalize(decoder.DecodeAll().data);
    EXPECT_LE(Deviation(expect_wav, actual_wav), max_std);
    EXPECT_FALSE(decoder.HasNext());
  }
}
