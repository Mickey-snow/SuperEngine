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

#include "base/avdec/nwa.h"
#include "test_utils.h"

#include <cstdlib>
#include <fstream>
#include <string>

class NwaDecoderTest : public ::testing::Test {
 protected:
  static constexpr float duration = 0.2;
  static constexpr int channels = 2;
  static constexpr int freq = 22050;
  static constexpr int samplesPerChannel = freq * duration;

  int16_t GetSampleAt(float t) {
    static constexpr auto pi = 3.1415926535897932384626433832795;
    static const float freq[] = {440, 523.25, 349.23};
    static const float amp[] = {0.5, 0.3, 0.2};
    static const float phase[] = {0, 0, pi / 2};
    float sample = 0;
    for (int i = 0; i < 3; ++i)
      sample += amp[i] * sin(2 * pi * freq[i] * t + phase[i]);
    return sample * INT16_MAX;
  };

  std::vector<int16_t> GetExpectedPcm() {
    static std::vector<int16_t> expect_wav;
    if (expect_wav.empty()) {
      expect_wav.reserve(samplesPerChannel);
      for (int i = 0; i < samplesPerChannel; ++i) {
        float t = 1.0 / freq * i;
        expect_wav.push_back(GetSampleAt(t));
      }
    }
    return expect_wav;
  }

  std::vector<int16_t> ToInt16Vec(const std::vector<uint8_t>& stream) {
    std::vector<int16_t> ret;
    ret.resize(stream.size() / 2);
    std::copy_n(stream.data(), stream.size(), (uint8_t*)ret.data());
    return ret;
  }

  std::vector<char> LoadFile(const std::string& filename) {
    std::vector<char> audioData;
    std::ifstream fp(filename);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
    return audioData;
  }

  std::tuple<std::vector<int16_t>, std::vector<int16_t>> SplitChannels(
      std::vector<int16_t> samples) {
    std::vector<int16_t> ch[2];
    int current_channel = 0;
    for (const auto& it : samples) {
      ch[current_channel].push_back(it);
      current_channel ^= 1;
    }
    return std::make_tuple(std::move(ch[0]), std::move(ch[1]));
  }

  double Deviation(std::vector<int16_t> a, std::vector<int16_t> b) {
    auto sqr = [](double x) { return x * x; };
    size_t n = a.size();
    double var = 0;
    for (size_t i = 0; i < n; ++i)
      var += sqr(a[i] - b[i]) / n;
    return sqrt(var);
  }
};

TEST_F(NwaDecoderTest, NoCompress) {
  const std::string filename = "Gameroot/BGM/BGM01.nwa";
  const float maxstd = 1e-4 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress2) {
  const std::string filename = "Gameroot/BGM/BGM02.nwa";
  const float maxstd = 0.02 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress1) {
  const std::string filename = "Gameroot/BGM/BGM03.nwa";
  const float maxstd = 0.05 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress0) {
  const std::string filename = "Gameroot/BGM/BGM04.nwa";
  const float maxstd = 0.025 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress3) {
  const std::string filename = "Gameroot/BGM/BGM05.nwa";
  const float maxstd = 0.0035 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress4) {
  const std::string filename = "Gameroot/BGM/BGM06.nwa";
  const float maxstd = 0.001 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}

TEST_F(NwaDecoderTest, Compress5) {
  const std::string filename = "Gameroot/BGM/BGM07.nwa";
  const float maxstd = 0.0007 * INT16_MAX;

  std::vector<char> rawdata = LoadFile(locateTestCase(filename));
  NwaDecoder decoder(rawdata.data(), rawdata.size());

  auto [lch, rch] = SplitChannels(ToInt16Vec(decoder.DecodeAll()));
  auto expect_wav = GetExpectedPcm();

  size_t n = expect_wav.size();
  ASSERT_EQ(lch.size(), n);
  ASSERT_EQ(rch.size(), n);

  EXPECT_LE(Deviation(lch, expect_wav), maxstd);
  EXPECT_LE(Deviation(rch, expect_wav), maxstd);
}
