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

#include <gtest/gtest.h>

#include "systems/base/voice_archive.h"
#include "systems/base/voice_cache.h"
#include "test_utils.h"
#include "utilities/bytestream.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

class VoiceCacheTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    testdir = PathToTestDirectory("Gameroot") / "vcache";
    fs::create_directories(testdir);

    fs::path baseDir = testdir / "ogg";
    for (int i = 400; i < 450; ++i)
      Touch(baseDir / (Encode_fileno(i) + ".ovk"));

    baseDir = testdir / "koe";
    for (int i = 1000; i < 1050; ++i)
      Touch(baseDir / (Encode_fileno(i) + ".koe"));

    baseDir = testdir / "nwa";
    for (int i = 2100; i < 2150; ++i)
      Touch(baseDir / (Encode_fileno(i) + ".nwk"));

    assets = std::make_shared<AssetScanner>();
    assets->IndexDirectory(testdir);

    SetUpOvk();
    SetUpNwk();
  }

  static void SetUpOvk() {
    oBytestream obs;
    obs << 2 << 6 << 0x24 << 20 << 0;
    obs << 7 << 0x2b << 40 << 0;
    obs << "Hello, wworld!";
    auto data = obs.Get();

    std::ofstream ovk414(testdir / "ogg" / "z0414.ovk",
                         std::ios::out | std::ios::binary);
    if (!ovk414.is_open())
      FAIL() << "Failed to open ovk414";
    ovk414.write(reinterpret_cast<const char*>(data.data()), data.size());
  }

  static void SetUpNwk() {
    oBytestream obs;
    obs << 2;
    obs << 7 << 0x2b << 66;
    obs << 6 << 0x24 << 33;
    obs << "aaaaeeee" << "Hello, wworld!";
    auto data = obs.Get();

    std::ofstream nwk2101(testdir / "nwa" / "z2101.nwk",
                          std::ios::out | std::ios::binary);
    if (!nwk2101.is_open())
      FAIL() << "Failed to open nwk2101";
    nwk2101.write(reinterpret_cast<const char*>(data.data()), data.size());
  }

  static void TearDownTestSuite() { fs::remove_all(testdir); }

  static std::string Encode_fileno(int file_no) {
    std::ostringstream oss;
    oss << 'z' << std::setw(4) << std::setfill('0') << file_no;
    return oss.str();
  }

  static std::string Encode_index(int index) {
    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << index;
    return oss.str();
  }

  static void Touch(fs::path path) {
    try {
      fs::create_directories(path.parent_path());
      std::ignore = std::ofstream(path);
    } catch (fs::filesystem_error& e) {
      FAIL() << "Filesystem error: " << e.what();
    }
  }

  inline static fs::path testdir;
  inline static std::shared_ptr<AssetScanner> assets;
};

TEST_F(VoiceCacheTest, LocateOvk) {
  VoiceCache vc(assets);

  auto basedir = testdir / "ogg";
  EXPECT_EQ(vc.LocateArchive(447), basedir / "z0447.ovk");
  ASSERT_EQ(vc.LocateArchive(414), basedir / "z0414.ovk");

  auto archive = vc.FindArchive(414);
  EXPECT_EQ(archive->LoadContent(20).Read(), "Hello,");
  EXPECT_EQ(archive->LoadContent(40).Read(), "wworld!");
  EXPECT_THROW(archive->LoadContent(18), std::runtime_error);
  EXPECT_THROW(archive->LoadContent(41), std::runtime_error);
}

TEST_F(VoiceCacheTest, LocateNwk) {
  VoiceCache vc(assets);

  auto basedir = testdir / "nwa";
  EXPECT_EQ(vc.LocateArchive(2149), basedir / "z2149.nwk");
  ASSERT_EQ(vc.LocateArchive(2101), basedir / "z2101.nwk");

  auto archive = vc.FindArchive(2101);
  EXPECT_EQ(archive->LoadContent(33).Read(), "Hello,");
  EXPECT_EQ(archive->LoadContent(66).Read(), "wworld!");
  EXPECT_THROW(archive->LoadContent(20), std::runtime_error);
  EXPECT_THROW(archive->LoadContent(40), std::runtime_error);
}
