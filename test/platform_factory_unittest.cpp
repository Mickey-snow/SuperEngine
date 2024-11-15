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

#include "platforms/implementor.hpp"
#include "platforms/platform_factory.hpp"

#include <filesystem>
#include <string>

class GoodPlatform : public IPlatformImplementor {
 public:
  GoodPlatform() = default;

  std::filesystem::path SelectGameDirectory() override {
    log += "SelectGameDirectory()\n";
    return {};
  }

  void ReportFatalError(const std::string& _1, const std::string& _2) override {
    log += "ReportFatalError(" + _1 + ',' + _2 + ")\n";
  }

  bool AskUserPrompt(const std::string& _1,
                     const std::string& _2,
                     const std::string& _3,
                     const std::string& _4) override {
    log += "AskUserPrompt(" + _1 + ',' + _2 + ',' + _3 + ',' + _4 + ")\n";
    return true;
  }

  inline static std::string log;
};

class BadPlatform : public IPlatformImplementor {
 public:
  BadPlatform() = default;

  std::filesystem::path SelectGameDirectory() override {
    Fail();
    return {};
  }

  void ReportFatalError(const std::string& _1, const std::string& _2) override {
    Fail();
  }

  bool AskUserPrompt(const std::string& _1,
                     const std::string& _2,
                     const std::string& _3,
                     const std::string& _4) override {
    Fail();
    return true;
  }

 private:
  void Fail() {
    GTEST_FAIL() << "class `BadPlatform` should not be instantiated.";
    throw std::runtime_error("Instantiation of BadPlatform");
  }
};

using std::string_literals::operator""s;

class PlatformFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PlatformFactory::Registrar(
        "Good Platform", []() { return std::make_shared<GoodPlatform>(); });
    PlatformFactory::Registrar(
        "Bad Platform", []() { return std::make_shared<BadPlatform>(); });
    PlatformFactory::Registrar(
        "My Platform", []() { return std::make_shared<GoodPlatform>(); });
  }

  void TearDown() override { PlatformFactory::Reset(); }
};

TEST_F(PlatformFactoryTest, CreateMyPlatform) {
  const std::string name = "My Platform";

  auto platform = PlatformFactory::Create(name);
  GoodPlatform::log.clear();
  EXPECT_NO_THROW(platform->ReportFatalError("err", "msg"));
  EXPECT_EQ(platform->SelectGameDirectory(), std::filesystem::path{});
  EXPECT_TRUE(platform->AskUserPrompt("text", "msg", "yes", "no"));

  EXPECT_EQ(GoodPlatform::log,
            "ReportFatalError(err,msg)\n"
            "SelectGameDirectory()\n"
            "AskUserPrompt(text,msg,yes,no)\n");
}

TEST_F(PlatformFactoryTest, DoubleRegister) {
  const std::string existing_name = "Good Platform";

  EXPECT_THROW(PlatformFactory::Registrar(
                   existing_name, []() -> PlatformImpl_t { return nullptr; }),
               std::invalid_argument);
}

TEST_F(PlatformFactoryTest, CreateDefaultPlatform) {
  auto platform = PlatformFactory::Create("default");

  auto good_platform = std::dynamic_pointer_cast<GoodPlatform>(platform);
  auto bad_platform = std::dynamic_pointer_cast<BadPlatform>(platform);
  EXPECT_TRUE(good_platform == nullptr && bad_platform == nullptr);

  PlatformFactory::Reset();
  EXPECT_NE(PlatformFactory::Create("default"), nullptr);
}
