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
#include "xclannad/wavfile.h"

#include <cstdlib>
#include <fstream>

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
  NwaDecoder dec(audioData.data(), audioData.size());

  std::vector<float> decoded;
  for (const auto& sample : dec.DecodeAll()) {
    EXPECT_LE(sample, 1.0);
    EXPECT_GE(sample, -1.0);
    decoded.push_back(sample * INT16_MAX);
  }

  auto n = decoded.size();
  ASSERT_EQ(n, freq * duration * channels);

  std::vector<float> expect_wav;
  for (int i = 0; i < n; ++i) {
    float t = 1.0 / freq * i;
    auto sample = 32767 * SampleAt(t);
    expect_wav.push_back(sample);
    expect_wav.push_back(sample);
  }

  auto sqr = [](auto x) { return x * x; };

  double var = 0;
  for (int i = 0; i < n; ++i)
    var += 1.0 * sqr(expect_wav[i] - decoded[i]) / n;
  EXPECT_LE(sqrt(var), 100);
}
