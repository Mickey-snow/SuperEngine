// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
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

#include "core/gameexe.hpp"

#include "test_utils.hpp"
#include "utilities/string_utilities.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

class GameexeTest : public ::testing::Test {
 protected:
  static Gameexe LoadGameexeOrFail(const std::filesystem::path& path) {
    auto loaded = Gameexe::FromFile(path);
    if (!loaded) {
      ADD_FAILURE() << "Failed to load Gameexe: " << loaded.error().message;
      return Gameexe();
    }
    return std::move(loaded.value());
  }

  static Gameexe LoadTestCase(std::string_view relative_path) {
    return LoadGameexeOrFail(LocateTestCase(std::string(relative_path)));
  }

  struct ValArr {
    std::vector<std::variant<int, std::string>> values;

    inline static std::string ToStr(const std::variant<int, std::string>& val) {
      if (auto* as_int = std::get_if<int>(&val))
        return std::to_string(*as_int);
      if (auto* as_str = std::get_if<std::string>(&val))
        return *as_str;
      return "cannot convert to string";
    }

    template <typename... Ts>
    inline ValArr(Ts&&... vals) : values{std::forward<Ts>(vals)...} {}

    friend bool operator==(const GameexeInterpretObject& gexe,
                           const ValArr& expected) {
      for (size_t i = 0; i < expected.values.size(); ++i) {
        auto const& it = expected.values[i];
        if (auto* as_str = std::get_if<std::string>(&it)) {
          auto actual = gexe.StrAt(i);
          if (!actual.has_value() || actual.value() != *as_str)
            return false;
        }
        if (auto* as_int = std::get_if<int>(&it)) {
          auto actual = gexe.IntAt(i);
          if (!actual.has_value() || actual.value() != *as_int)
            return false;
        }
      }
      return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const ValArr& me) {
      return os << Join(
                 ",", std::ranges::transform_view(
                          me.values, [](const auto& it) { return ToStr(it); }));
    }
  };
};

TEST_F(GameexeTest, ReadAllKeys) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  EXPECT_EQ(static_cast<size_t>(26), ini.Size()) << "Wrong number of keys";
}

TEST_F(GameexeTest, SiglusFormatParsing) {
  Gameexe ini = LoadTestCase("Gameexe_data/siglus.ini");

  EXPECT_EQ(ini("BGM.000"), ValArr("BGM01", "BGM01", 82286, 5184000, 905143));

  EXPECT_EQ(ini("CONFIG.FONT.NAME"), ValArr("Noto Serif JP Medium"));
  EXPECT_EQ(ini("CONFIG.SWITCH.ON"), ValArr(1));
  EXPECT_EQ(ini("CHR.ENTRY"), ValArr("Hero", "", 1, 255));
  EXPECT_EQ(ini("FLAGS"), ValArr(-1, 0, 1));
  EXPECT_EQ(ini("TEXT.LINE"), ValArr("Quoted value", "next"));
}

TEST_F(GameexeTest, RealliveFormatParsing) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  EXPECT_EQ(ini("CAPTION"), ValArr("Canon: A firearm"));
  EXPECT_FALSE(ini("RANDOM_KEY").Exists()) << "#RANDOM_KEY should not exist";
  EXPECT_EQ(ini("WINDOW_ATTR"), ValArr(1, 2, 3, 4, 5));
}

TEST_F(GameexeTest, MultipleKeys) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  EXPECT_EQ(1, ini("IMAGINE", "ONE").Int());
  EXPECT_EQ(2, ini("IMAGINE", "TWO").Int());
  EXPECT_EQ(3, ini("IMAGINE", "THREE").Int());

  ini("IMAGINE", "ONE") = 4;
  ini("IMAGINE", "TWO") = 5;
  ini("IMAGINE", "THREE") = 6;
  EXPECT_EQ(4, ini("IMAGINE", "ONE").Int());
  EXPECT_EQ(5, ini("IMAGINE", "TWO").Int());
  EXPECT_EQ(6, ini("IMAGINE", "THREE").Int());
}

TEST_F(GameexeTest, ChainingWorks) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  auto imagine = ini("IMAGINE");
  EXPECT_EQ(1, imagine("ONE").Int());
  EXPECT_EQ(2, imagine("TWO").Int());
  EXPECT_EQ(3, imagine("THREE").Int());

  imagine("ONE") = -100;
  imagine("ONE") = 7;
  imagine("TWO") = 8;
  imagine("THREE") = 9;
  imagine("FOUR") = 10;
  EXPECT_EQ(7, ini("IMAGINE", "ONE").Int());
  EXPECT_EQ(8, ini("IMAGINE", "TWO").Int());
  EXPECT_EQ(9, ini("IMAGINE", "THREE").Int());
  EXPECT_EQ(10, ini("IMAGINE", "FOUR").Int());
  EXPECT_EQ(7, imagine("ONE").Int());
  EXPECT_EQ(8, imagine("TWO").Int());
  EXPECT_EQ(9, imagine("THREE").Int());
  EXPECT_EQ(10, imagine("FOUR").Int());
}

TEST_F(GameexeTest, FilterRange) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");

  {
    std::vector<int> expected{1, 3, 2};
    std::vector<int> actual;
    for (auto entry : ini.Filter("IMAGINE."))
      actual.push_back(entry.Int().value());
    EXPECT_EQ(expected, actual);
  }

  {
    std::multiset<int> expected{-1, 0, 0, 0, 0, 0, 1, 2, 22, 25, 42, 90};
    std::multiset<int> actual;
    for (auto entry : ini.Filter("WINDOW."))
      actual.insert(entry.Int().value());
    EXPECT_EQ(expected, actual);
  }
}

TEST_F(GameexeTest, MultipleIterate) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");

  const std::vector<int> expected{1, 3, 2};
  auto filter_range = ini.Filter("IMAGINE");
  for (int i = 0; i < 10; ++i) {
    std::vector<int> actual;
    for (auto entry : filter_range)
      actual.push_back(entry.Int().value());
    EXPECT_EQ(expected, actual);
  }
}

TEST_F(GameexeTest, FilterEmpty) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  for (const auto entry : ini.Filter("nonexist.OBJECT"))
    FAIL() << "Filter should be empty";
}

TEST_F(GameexeTest, KeyParts) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  auto gio = ini("WINDOW.000.ATTR_MOD");
  std::vector<std::string> pieces = gio.GetKeyParts();
  ASSERT_EQ(3u, pieces.size());
  EXPECT_EQ("WINDOW", pieces[0]);
  EXPECT_EQ("000", pieces[1]);
  EXPECT_EQ("ATTR_MOD", pieces[2]);
}

TEST_F(GameexeTest, DstrackRegression) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe_tokenization.ini");

  EXPECT_EQ(ini("CLANNADDSTRACK"),
            ValArr(0, 99999999, 269364, "BGM01", "BGM01"));
  EXPECT_EQ(ini("DCDSTRACK"), ValArr(0, 10998934, 0, "dcbgm000", "dcbgm000"));
}

TEST_F(GameexeTest, MissingKeyReturnsError) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  auto missing = ini("DOES_NOT_EXIST").Int();
  ASSERT_FALSE(missing);
  EXPECT_EQ("Unknown Gameexe key", missing.error().message);
}

TEST_F(GameexeTest, TypeMismatchReturnsError) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  auto mismatch = ini("CAPTION").Int();
  ASSERT_FALSE(mismatch);
  EXPECT_EQ("Value is not an integer", mismatch.error().message);
}

TEST_F(GameexeTest, ExpectIntVectorRejectsStrings) {
  Gameexe ini = LoadTestCase("Gameexe_data/Gameexe.ini");
  auto result = ini("CAPTION").IntVec();
  ASSERT_FALSE(result);
  EXPECT_EQ("Value is not an integer", result.error().message);
}

TEST_F(GameexeTest, LoadInvalidFileReportsError) {
  auto invalid =
      Gameexe::FromFile(LocateTestCase("Gameexe_data/Gameexe_invalid.ini"));
  ASSERT_FALSE(invalid);
  EXPECT_EQ("Missing '=' delimiter in Gameexe line", invalid.error().message);
  ASSERT_TRUE(invalid.error().line.has_value());
  EXPECT_EQ(static_cast<std::size_t>(2), invalid.error().line.value());
}
