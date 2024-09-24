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

#include "base/avdec/image_decoder.h"
#include "base/compression.h"
#include "test_utils.h"
#include "utilities/bytestream.h"
#include "utilities/mapped_file.h"
#include "xclannad/file.h"

#include <cstring>
#include <random>
#include <string>
#include <vector>

TEST(g00Test, MonochromaticGrad) {
  MappedFile file(PathToTestCase("Gameroot/g00/monochrome.g00"));
  ImageDecoder dec(file.Read());

  auto width = dec.width;
  auto height = dec.height;
  ASSERT_EQ(width, 16);
  ASSERT_EQ(height, 8);
  EXPECT_EQ(dec.region_table.size(), 0);

  auto result = dec.mem.data();
  for (int j = 0; j < height; ++j)
    for (int i = 0; i < width; ++i) {
      float fr = static_cast<float>(i) / width;
      float to = static_cast<float>(i + 1) / width;

      fr *= 255.0f;
      to *= 255.0f;
      for (int k = 0; k < 3; ++k) {
        EXPECT_TRUE(fr <= *result && *result <= to);
        ++result;
      }
      EXPECT_EQ(*result++, 0xff);
    }
}

TEST(g00Test, ChromaticGrad) {
  MappedFile file(PathToTestCase("Gameroot/g00/rainbow.g00"));
  ImageDecoder dec(file.Read());

  const auto height = dec.height;
  const auto width = dec.width;
  ASSERT_EQ(width, 16);
  ASSERT_EQ(height, 8);
  EXPECT_EQ(dec.region_table.size(), 0);

  char* result = dec.mem.data();
  for (int j = 0; j < height; ++j)
    for (int i = 0; i < width; ++i) {
      float val =
          (static_cast<float>(j * height) + static_cast<float>(i * width)) /
          static_cast<float>(height * height + width * width);
      float r = val * 255.0f;
      float g = (1.0f - val) * 255.0f;
      float b = val * 255.0f;
      EXPECT_NEAR(*result++, r, 13.0f);
      EXPECT_NEAR(*result++, g, 13.0f);
      EXPECT_NEAR(*result++, b, 13.0f);
      EXPECT_EQ(*result++, 0xff);
    }
}

TEST(g00Test, RegionTable) {
  MappedFile file(PathToTestCase("Gameroot/g00/rainbow_cut.g00"));
  ImageDecoder dec(file.Read());

  const auto height = dec.height;
  const auto width = dec.width;
  ASSERT_EQ(width, 16);
  ASSERT_EQ(height, 8);
  ASSERT_EQ(dec.region_table.size(), 8);
  for (int j = 0; j < 2; ++j)
    for (int i = 0; i < 4; ++i) {
      auto rect = dec.region_table[j * 4 + i].rect;
      const Point upleft(i * 4, j * 4), downright((i + 1) * 4, (j + 1) * 4);
      EXPECT_EQ(rect, Rect(upleft, downright));
    }
}

std::string PackLzss(std::string_view data) {
  std::string packed;
  packed.reserve(data.size() + 8 + data.size() / 8);
  for (size_t i = 0; i < data.size(); ++i) {
    if (i % 8 == 0)
      packed += static_cast<char>(0xff);
    packed += data[i];
  }
  oBytestream obs;
  obs << static_cast<uint32_t>(packed.size() + 8)
      << static_cast<uint32_t>(data.size());
  auto v1 = obs.Get();
  return std::string(reinterpret_cast<char*>(v1.data()), v1.size()) + packed;
}

TEST(g00Test, IndexColor) {
  constexpr uint32_t h = 128, w = 128;
  int colorAt[h][w];
  uint32_t palette[256];

  std::mt19937 gen;
  for (int i = 0; i < 256; ++i)
    palette[i] = gen();

  for (uint32_t j = 0; j < h; ++j)
    for (uint32_t i = 0; i < w; ++i) {
      static std::uniform_int_distribution<> dist(0, 255);
      colorAt[j][i] = dist(gen);
    }

  auto Prepare_data = [&]() {
    oBytestream obs;
    obs << static_cast<uint8_t>(1) << static_cast<uint16_t>(w)
        << static_cast<uint16_t>(h);
    obs << PackLzss([&]() -> std::string {
      oBytestream obs;
      obs << static_cast<uint16_t>(256);
      for (int i = 0; i < 256; ++i)
        obs << palette[i];

      for (uint32_t j = 0; j < h; ++j)
        for (uint32_t i = 0; i < w; ++i)
          obs << static_cast<uint8_t>(colorAt[j][i]);
      auto data = obs.Get();
      return std::string(reinterpret_cast<char*>(data.data()), data.size());
    }());

    std::vector<uint8_t> data_vec = obs.Get();
    return std::string(reinterpret_cast<const char*>(data_vec.data()),
                       data_vec.size());
  };

  std::string data = Prepare_data();
  ImageDecoder dec(data);
  const auto height = dec.height;
  const auto width = dec.width;
  EXPECT_EQ(height, h);
  EXPECT_EQ(width, w);

  uint32_t* result = reinterpret_cast<uint32_t*>(dec.mem.data());
  for (uint32_t j = 0; j < h; ++j)
    for (uint32_t i = 0; i < w; ++i)
      EXPECT_EQ(*result++, palette[colorAt[j][i]]);
}
