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

#include "base/avdec/wav.h"
#include "libreallive/filemap.h"

#include "test_utils.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <string>
#include <type_traits>

template <class T>
struct remove_cvref {
  using type = std::remove_const_t<std::remove_reference_t<T>>;
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

namespace fs = std::filesystem;
using libreallive::MappedFile;

class WavDecoderTest : public ::testing::TestWithParam<std::string> {
 public:
  void SetUp() override {
    const std::string filepath = GetParam();
    {
      std::ifstream fs(filepath + ".param");
      fs >> sample_rate >> channel >> sample_width >> frequency >> duration;
    }
    {
      std::ifstream fs(filepath);
      fs.seekg(0, std::ios::end);
      size_t n = fs.tellg();
      fs.seekg(0, std::ios::beg);

      data.resize(n);
      fs.read(data.data(), n);
    }
  }

  AV_SAMPLE_FMT GetSampleFormat() {
    switch (sample_width) {
      case 1:
        return AV_SAMPLE_FMT::S8;
      case 2:
        return AV_SAMPLE_FMT::S16;
      case 4:
        return AV_SAMPLE_FMT::S32;
      case 8:
        return AV_SAMPLE_FMT::S64;
      default:
        return AV_SAMPLE_FMT::NONE;
    }
  }

  AudioData ReproduceAudio() {
    AudioData result;
    result.spec = {.sample_rate = sample_rate,
                   .sample_format = GetSampleFormat(),
                   .channel_count = channel};

    std::vector<double> wav;
    for (int i = 0; i < sample_rate * duration; ++i) {
      float t = 1.0 * i / sample_rate;
      static float pi = acos(-1.0);
      double sample = sin(2 * pi * frequency * t);
      for (int i = 0; i < channel; ++i)
        wav.push_back(sample);
    }

    result.data = wav;
    return result;
  }

  std::vector<double> Normalize(avsample_buffer_t& a) {
    return std::visit(
        [](auto& a) -> std::vector<double> {
          using sample_t = typename remove_cvref_t<decltype(a)>::value_type;
          const auto max_value =
              static_cast<double>(std::numeric_limits<sample_t>::max());

          std::vector<double> result;
          for (const auto& it : a)
            result.push_back(static_cast<double>(it) / max_value);
          return result;
        },
        a);
  }

  double Deviation(const std::vector<double>& a, const std::vector<double>& b) {
    auto sqr = [](auto x) { return x * x; };
    size_t n = a.size();
    double var = 0;
    for (size_t i = 0; i < n; ++i)
      var += sqr(a[i] - b[i]) / n;
    return sqrt(var);
  }

  std::vector<char> data;
  int sample_rate, channel, sample_width;
  int frequency;
  float duration;
};

TEST_P(WavDecoderTest, DecodeWav) {
  static constexpr double max_std = 1e-3;

  WavDecoder decoder(std::string_view(data.data(), data.size()));

  AudioData result = decoder.DecodeAll();
  auto expect = ReproduceAudio();
  ASSERT_EQ(expect.spec, result.spec);
  ASSERT_EQ(expect.SampleCount(), result.SampleCount());

  auto expect_wav = std::get<std::vector<double>>(expect.data);
  auto result_wav = Normalize(result.data);
  EXPECT_LE(Deviation(expect_wav, result_wav), max_std);
}

std::vector<std::string> GetTestWavFiles() {
  static const std::string testdir = locateTestDirectory("Gameroot/WAV");
  static const std::regex pattern(".*test[0-9]+\\.wav");

  std::vector<std::string> testFiles;
  for (const auto& entry : fs::directory_iterator(testdir)) {
    auto entryPath = entry.path().string();
    if (entry.is_regular_file() && std::regex_match(entryPath, pattern))
      testFiles.push_back(entryPath);
  }
  return testFiles;
}

using ::testing::ValuesIn;
INSTANTIATE_TEST_SUITE_P(WavDecoderDataTest,
                         WavDecoderTest,
                         ValuesIn(GetTestWavFiles()),
                         ([](const auto& info) {
                           std::string name = info.param;
                           size_t begin = name.rfind('/') + 1,
                                  end = name.rfind('.');
                           return name.substr(begin, end - begin);
                         }));
