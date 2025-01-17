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

#include "core/asset_scanner.hpp"
#include "core/gameexe.hpp"
#include "test_utils.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class rlfsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rlvm_extension =
        std::set<std::string>{"g00", "pdt", "anm", "gan", "hik", "wav",
                              "ogg", "nwa", "mp3", "ovk", "koe", "nwk"};
    nonrlvm_extension =
        std::set<std::string>{"docx", "pdf",  "html", "svg", "csv",
                              "tiff", "pptx", "g01",  "g0",  "nnwa"};

    emptydir = PathToTestDirectory("Gameroot") / "EmptyDir";
    fs::create_directories(emptydir);
    extradir = PathToTestDirectory("Gameroot") / "Extra";
    fs::create_directories(extradir);
  }

  void TearDown() override {
    fs::remove_all(emptydir);
    fs::remove_all(extradir);
  }

  AssetScanner game_assets;
  std::set<std::string> rlvm_extension, nonrlvm_extension;
  fs::path emptydir, extradir;  // tmp directories under Gameroot for testing
};

TEST_F(rlfsTest, IndexDirectory) {
  game_assets.IndexDirectory(PathToTestDirectory("Gameroot"));

  EXPECT_EQ(game_assets.FindFile("bgm01"),
            PathToTestCase("Gameroot/BGM/BGM01.nwa"));
  EXPECT_EQ(game_assets.FindFile("doesntmatter", rlvm_extension),
            PathToTestCase("Gameroot/g00/doesntmatter.g00"));
  EXPECT_THROW(game_assets.FindFile("BGM01", nonrlvm_extension),
               std::runtime_error);

  EXPECT_THROW(game_assets.FindFile("nosuchfile"), std::runtime_error);
}

TEST_F(rlfsTest, BuildFromGexe) {
  Gameexe gexe(PathToTestCase("Gameexe_data/rl_filesystem.ini"));
  gexe("__GAMEPATH") = LocateTestDirectory("Gameroot");

  game_assets = AssetScanner(gexe);

  EXPECT_EQ(game_assets.FindFile("bgm01"),
            PathToTestCase("Gameroot/BGM/BGM01.nwa"));
  EXPECT_EQ(game_assets.FindFile("doesntmatter", rlvm_extension),
            PathToTestCase("Gameroot/g00/doesntmatter.g00"));
  EXPECT_THROW(game_assets.FindFile("BGM01", nonrlvm_extension),
               std::runtime_error);

  EXPECT_THROW(game_assets.FindFile("nosuchfile"), std::runtime_error);
}

TEST_F(rlfsTest, EmptyDir) {
  EXPECT_NO_THROW(game_assets.IndexDirectory(emptydir));
  EXPECT_NO_THROW(game_assets.IndexDirectory(emptydir, rlvm_extension));
  EXPECT_THROW(game_assets.FindFile("nonexistentfile"), std::runtime_error);
}

TEST_F(rlfsTest, SpecialFiles) {
  fs::path specialnwa = extradir / "@special!.nwa";
  fs::path hiddeng00 = extradir / ".hidden.g00";

  {
    std::ofstream a(specialnwa);
    std::ofstream b(hiddeng00);
    std::ofstream c(extradir / "abc...");
    std::ofstream d(extradir / "noextension!!!");
  }

  game_assets.IndexDirectory(extradir, rlvm_extension);
  EXPECT_EQ(game_assets.FindFile("@special!"), specialnwa);
  EXPECT_EQ(game_assets.FindFile(".hidden"), hiddeng00);
  EXPECT_THROW(game_assets.FindFile("abc"), std::runtime_error);
  EXPECT_THROW(game_assets.FindFile("noextension!!!"), std::runtime_error);
}

TEST_F(rlfsTest, InvalidInput) {
  EXPECT_THROW(game_assets.IndexDirectory(PathToTestDirectory("Gameroot") /
                                          "InvalidDir"),
               std::invalid_argument);

  // Pass an invalid Gameexe configuration
  Gameexe invalidGexe;
  invalidGexe("__GAMEPATH") = "";
  EXPECT_THROW(game_assets.BuildFromGameexe(invalidGexe), std::runtime_error);
}
