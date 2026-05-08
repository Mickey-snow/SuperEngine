// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "utilities/static_registry.hpp"

#include <stdexcept>
#include <string>

struct TestRegistryTag;
using TestRegistry = StaticRegistry<TestRegistryTag, std::string, int>;

struct OtherRegistryTag;
using OtherRegistry = StaticRegistry<OtherRegistryTag, std::string, int>;

struct MacroRegistryTag;
using MacroRegistry = StaticRegistry<MacroRegistryTag, std::string, int>;

RLVM_REGISTER(MacroRegistry, "macro", 42)

TEST(StaticRegistryTest, RegisterAndFind) {
  TestRegistry::Reset();

  TestRegistry::Register("alpha", 1);

  const int* value = TestRegistry::Find("alpha");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, 1);
  EXPECT_EQ(TestRegistry::Find("missing"), nullptr);

  TestRegistry::Reset();
}

TEST(StaticRegistryTest, ContainerQueries) {
  TestRegistry::Reset();

  EXPECT_TRUE(TestRegistry::empty());
  EXPECT_EQ(TestRegistry::size(), 0);

  TestRegistry::Register("alpha", 1);
  TestRegistry::Register("beta", 2);

  EXPECT_FALSE(TestRegistry::empty());
  EXPECT_EQ(TestRegistry::size(), 2);
  EXPECT_TRUE(TestRegistry::Contains("alpha"));
  EXPECT_FALSE(TestRegistry::Contains("missing"));

  auto it = TestRegistry::cbegin();
  ASSERT_NE(it, TestRegistry::cend());
  EXPECT_EQ(it->first, "alpha");
  EXPECT_EQ(it->second, 1);

  TestRegistry::Reset();
}

TEST(StaticRegistryTest, DuplicateRegistrationThrows) {
  TestRegistry::Reset();

  TestRegistry::Register("alpha", 1);

  EXPECT_THROW(TestRegistry::Register("alpha", 2), std::invalid_argument);

  TestRegistry::Reset();
}

TEST(StaticRegistryTest, ResetClearsRegistry) {
  TestRegistry::Reset();

  TestRegistry::Register("alpha", 1);
  TestRegistry::Reset();

  EXPECT_TRUE(TestRegistry::empty());
  EXPECT_EQ(TestRegistry::Find("alpha"), nullptr);
}

TEST(StaticRegistryTest, TagsCreateIndependentRegistries) {
  TestRegistry::Reset();
  OtherRegistry::Reset();

  TestRegistry::Register("same", 1);
  OtherRegistry::Register("same", 2);

  ASSERT_NE(TestRegistry::Find("same"), nullptr);
  ASSERT_NE(OtherRegistry::Find("same"), nullptr);
  EXPECT_EQ(*TestRegistry::Find("same"), 1);
  EXPECT_EQ(*OtherRegistry::Find("same"), 2);

  TestRegistry::Reset();
  OtherRegistry::Reset();
}

TEST(StaticRegistryTest, MacroRegistersStaticValue) {
  const int* value = MacroRegistry::Find("macro");

  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, 42);
}
