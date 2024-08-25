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

#include "base/acodec/nwa.h"
#include "test_utils.h"

#include <cstdlib>
#include <fstream>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static float SampleAt(float t) {
  static constexpr auto pi = 3.1415926535897932384626433832795;
  static const float freq[] = {440, 523.25, 349.23};
  static const float amp[] = {0.5, 0.3, 0.2};
  static const float phase[] = {0, 0, pi / 2};
  float sample = 0;
  for (int i = 0; i < 3; ++i)
    sample += amp[i] * sin(2 * pi * freq[i] * t + phase[i]);
  return sample;
};

static constexpr float duration = 0.2;
static constexpr int channels = 2;
static constexpr int freq = 22050;

template <typename T>
static T sqr(T x) {
  return x * x;
}

TEST(NwaDecode, NoCompress) {
  fs::path audiofile = locateTestCase("Gameroot/BGM/BGM01.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }
  NwaDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }

  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 1e-4);
}

TEST(NwaDecode, Compress2) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM02.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.02);
}

TEST(NwaDecode, Compress1) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM03.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.05);
}

TEST(NwaDecode, Compress0) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM04.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.05);
}

TEST(NwaDecode, Compress3) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM05.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.0035);
}

TEST(NwaDecode, Compress4) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM06.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.001);
}

TEST(NwaDecode, Compress5) {
  std::string audiofile = locateTestCase("Gameroot/BGM/BGM07.nwa");
  std::vector<char> audioData;
  {
    std::ifstream fp(audiofile);
    fp.seekg(0, std::ios::end);
    auto n = fp.tellg();
    fp.seekg(0, std::ios::beg);
    audioData.resize(n);
    fp.read(audioData.data(), n);
  }

  NwaCompDecoder decoder(audioData.data(), audioData.size());

  std::vector<float> actual_sample[2];
  int ch = 0;
  for (const auto& it : decoder.DecodeAll()) {
    actual_sample[ch].push_back(it);
    ch ^= 1;
  }
  auto n = actual_sample[0].size();
  ASSERT_EQ(actual_sample[0].size(), actual_sample[1].size());
  ASSERT_EQ(n, freq * duration);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    expect_wav.push_back(SampleAt(t));
  }

  double var[2] = {};
  for (int i = 0; i < n; ++i) {
    var[0] += sqr(expect_wav[i] - actual_sample[0][i]) / n;
    var[1] += sqr(expect_wav[i] - actual_sample[1][i]) / n;
  }
  EXPECT_LE(sqrt((var[0] + var[1]) / 2), 0.0007);
}
