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

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace fs = std::filesystem;
using libreallive::MappedFile;

class WavDecoderTestWith : public ::testing::TestWithParam<std::string> {
 public:
  WavDecoderTestWith() : data(GetParam()) {}

  void SetUp() override {
    std::string filepath = GetParam();
    std::ifstream fs(filepath + ".param");
    fs >> sample_rate >> channel >> sample_width >> frequency >> duration;
  }

  MappedFile data;
  int sample_rate, channel, sample_width, frequency;
  float duration;
};

TEST_P(WavDecoderTestWith, init) {
  auto param = GetParam();
  std::cout << param << std::endl;
  EXPECT_FALSE(param.empty());
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
                         WavDecoderTestWith,
                         ValuesIn(GetTestWavFiles()),
                         ([](const auto& info) {
                           std::string name = info.param;
                           size_t begin = name.rfind('/') + 1,
                                  end = name.rfind('.');
                           return name.substr(begin, end - begin);
                         }));
