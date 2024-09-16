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

#include "base/voice_archive/ivoicearchive.h"
#include "base/voice_archive/nwk.h"
#include "base/voice_archive/ovk.h"
#include "base/voice_factory.h"
#include "test_utils.h"
#include "utilities/bytestream.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static std::random_device rd;
static std::mt19937 gen(rd());

class VoiceFactoryTest : public ::testing::Test {
 public:
  static fs::path SetUpTestDir(std::string baseName) {
    testdir = PathToTestDirectory("Gameroot") / "vcache";
    fs::create_directories(testdir);

    fs::path baseDir = testdir / baseName;
    for (int i = 0; i <= 100; ++i)
      Touch(baseDir / (Encode_fileno(i) + '.' + baseName));
    return baseDir;
  }

  static void TearDownTestDir() {
    if (fs::exists(testdir))
      fs::remove_all(testdir);
  }

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

  static int Encode_id(int file_no, int index) {
    return file_no * 100000 + index;
  }

  static void Touch(fs::path path) {
    try {
      fs::create_directories(path.parent_path());
      std::ignore = std::ofstream(path);
    } catch (fs::filesystem_error& e) {
      FAIL() << "Filesystem error: " << e.what();
    }
  }

  static std::vector<char> RandomVector() {
    static constexpr auto MAXSIZE = 100;
    std::uniform_int_distribution<> distrib(1, MAXSIZE);
    std::vector<char> ret(distrib(gen));
    std::generate(ret.begin(), ret.end(), [&]() { return distrib(gen); });
    return ret;
  }

  inline static fs::path testdir;
};

class OvkVoiceTest : public VoiceFactoryTest {
 protected:
  static void SetUpTestSuite() {
    base_dir = SetUpTestDir("ovk");
    SetUpOvk();
    assets = std::make_shared<AssetScanner>();
    assets->IndexDirectory(testdir);
  }

  static void TearDownTestSuite() { fs::remove_all(testdir); }

  static void SetUpOvk() {
    ovk_file_no = 14;
    ovk_archive_path = base_dir / "z0014.ovk";

    oBytestream obs;
    OVK_Header hdr[100];
    int loc = sizeof(int) + sizeof(hdr);

    for (int i = 0; i < 100; ++i) {
      ovk_voice[i] = RandomVector();
      int n = static_cast<int>(ovk_voice[i].size());
      hdr[i] = {.size = n, .offset = loc, .id = i, .sample_count = -1};
      loc += n;
    }
    std::shuffle(hdr, hdr + 100, gen);

    // write header
    obs << 100;
    for (int i = 0; i < 100; ++i)
      obs << hdr[i].size << hdr[i].offset << hdr[i].id << hdr[i].sample_count;

    auto data = obs.Get();
    std::ofstream ofs(ovk_archive_path, std::ios::out | std::ios::binary);
    if (!ofs.is_open())
      FAIL() << "Failed to open " << ovk_archive_path.string();
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());

    // write archive
    for (int i = 0; i < 100; ++i)
      ofs.write(ovk_voice[i].data(), ovk_voice[i].size());
  }

  inline static std::vector<char> ovk_voice[100];
  inline static fs::path ovk_archive_path, base_dir;
  inline static int ovk_file_no;
  inline static std::shared_ptr<AssetScanner> assets;
};

TEST_F(OvkVoiceTest, LocateArchive) {
  VoiceFactory vc(assets);

  EXPECT_EQ(vc.LocateArchive(47), base_dir / "z0047.ovk");
  EXPECT_EQ(vc.LocateArchive(4), base_dir / "z0004.ovk");
  EXPECT_EQ(vc.LocateArchive(14), ovk_archive_path);
}

TEST_F(OvkVoiceTest, LoadOggSample) {
  VoiceFactory vc(assets);

  for (int i = 0; i < 100; ++i) {
    VoiceClip sample = vc.LoadSample(Encode_id(ovk_file_no, i));
    EXPECT_EQ(sample.format_name, "ogg");
    auto expect_content =
        std::string_view(ovk_voice[i].data(), ovk_voice[i].size());
    EXPECT_EQ(sample.content.Read(), expect_content);
  }
}

class NwaVoiceTest : public VoiceFactoryTest {
 protected:
  static void SetUpTestSuite() {
    base_dir = SetUpTestDir("nwk");
    SetUpNwk();
    assets = std::make_shared<AssetScanner>();
    assets->IndexDirectory(testdir);
  }

  static void TearDownTestSuite() { TearDownTestDir(); }

  static void SetUpNwk() {
    nwk_file_no = 1;
    nwk_archive_path = base_dir / (Encode_fileno(nwk_file_no) + ".nwk");

    oBytestream obs;
    NWK_Header hdr[100];
    int loc = sizeof(int) + sizeof(hdr);

    for (int i = 0; i < 100; ++i) {
      nwk_voice[i] = RandomVector();
      int n = static_cast<int>(nwk_voice[i].size());
      hdr[i] = {.size = n, .offset = loc, .id = i};
      loc += n + 10;  // padding bytes
    }

    std::ofstream ofs(nwk_archive_path, std::ios::out | std::ios::binary);
    if (!ofs.is_open())
      FAIL() << "Failed to open " << nwk_archive_path.string();
    for (int i = 0; i < 100; ++i) {
      ofs.seekp(hdr[i].offset, std::ios::beg);
      ofs.write(nwk_voice[i].data(), nwk_voice[i].size());
    }

    obs << 100;
    std::shuffle(hdr, hdr + 100, gen);
    for (int i = 0; i < 100; ++i)
      obs << hdr[i].size << hdr[i].offset << hdr[i].id;
    auto data = obs.Get();
    ofs.seekp(0, std::ios::beg);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
  }

  inline static std::vector<char> nwk_voice[100];
  inline static fs::path base_dir, nwk_archive_path;
  inline static int nwk_file_no;
  inline static std::shared_ptr<AssetScanner> assets;
};

TEST_F(NwaVoiceTest, LocateArchive) {
  VoiceFactory vc(assets);

  EXPECT_EQ(vc.LocateArchive(49), base_dir / "z0049.nwk");
  EXPECT_EQ(vc.LocateArchive(11), base_dir / "z0011.nwk");
  EXPECT_EQ(vc.LocateArchive(1), nwk_archive_path);
}

TEST_F(NwaVoiceTest, LoadNwaSample) {
  VoiceFactory vc(assets);

  for (int i = 0; i < 100; ++i) {
    VoiceClip sample = vc.LoadSample(Encode_id(nwk_file_no, i));
    EXPECT_EQ(sample.format_name, "nwa");
    auto expect_content =
        std::string_view(nwk_voice[i].data(), nwk_voice[i].size());
    EXPECT_EQ(sample.content.Read(), expect_content);
  }
}

class OggVoiceTest : public VoiceFactoryTest {
 protected:
  static void SetUpTestSuite() {
    base_dir = SetUpTestDir("ogg");
    SetUpOgg();
    assets = std::make_shared<AssetScanner>();
    assets->IndexDirectory(testdir);
  }

  static void TearDownTestSuite() { TearDownTestDir(); }

  static void SetUpOgg() {
    ogg_file_no = 49;
    ogg_index = 73;
    ogg_voice = RandomVector();
    ogg_path = base_dir / "0049" / "z004900073.ogg";
    Touch(ogg_path);
    std::ofstream ofs(ogg_path, std::ios::out | std::ios::binary);
    if (!ofs.is_open())
      FAIL() << "Failed to open " << ogg_path.string();
    ofs.write(ogg_voice.data(), ogg_voice.size());
  }

  inline static std::vector<char> ogg_voice;
  inline static fs::path ogg_path, base_dir;
  inline static int ogg_file_no, ogg_index;
  inline static std::shared_ptr<AssetScanner> assets;
};

TEST_F(OggVoiceTest, LoadUnpackedSample) {
  VoiceFactory vc(assets);
  VoiceClip sample = vc.LoadSample(Encode_id(ogg_file_no, ogg_index));
  EXPECT_EQ(sample.format_name, "ogg");
  auto expect_content = std::string_view(ogg_voice.data(), ogg_voice.size());
  EXPECT_EQ(sample.content.Read(), expect_content);
}
