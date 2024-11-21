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

#include "libreallive/scriptor.hpp"

#include "libreallive/archive.hpp"
#include "libreallive/elements/bytecode.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace libreallive;

class MockBytecodeElement : public BytecodeElement {
 public:
  MOCK_METHOD(void,
              PrintSourceRepresentation,
              (IModuleManager*, std::ostream&),
              (const, override));

  MOCK_METHOD(std::string,
              GetSourceRepresentation,
              (IModuleManager*),
              (const, override));
  MOCK_METHOD(size_t, GetBytecodeLength, (), (const, override));
  MOCK_METHOD(std::string,
              GetSerializedCommand,
              (RLMachine&),
              (const, override));
  MOCK_METHOD(Bytecode_ptr, DownCast, (), (const, override));

  MockBytecodeElement(int pos) : pos_(pos) {}

  // hijack this method for testing
  int GetEntrypoint() const override { return pos_; }

  int pos_;
};

class MockArchive : public Archive {
 public:
  MOCK_METHOD((Scenario*), GetScenario, (int index), (override));
};

class ScriptorTest : public ::testing::Test {
 protected:
  ScriptorTest() : archive(), scriptor(archive) {}

  MockArchive archive;
  Scriptor scriptor;

  template <typename... Ts>
  Script MakeScript(Ts... locs) {
    std::vector<std::pair<unsigned long, std::shared_ptr<BytecodeElement>>>
        elements;
    (elements.emplace_back(locs, std::make_shared<MockBytecodeElement>(locs)),
     ...);
    return Script(std::move(elements), std::map<int, unsigned long>{});
  }

  std::vector<int> Traverse(Scriptor::const_iterator it) {
    std::vector<int> result;
    while (it.HasNext())
      result.push_back((*it++)->GetEntrypoint());
    return result;
  }
};

using ::testing::Return;

TEST_F(ScriptorTest, IterateForward) {
  Scenario s1({}, MakeScript(1, 2, 3), 1);
  EXPECT_CALL(archive, GetScenario(1)).WillRepeatedly(Return(&s1));

  auto it = scriptor.Load(1, 1);
  EXPECT_EQ(it.ScenarioNumber(), 1);
  EXPECT_EQ(Traverse(it), (std::vector{1, 2, 3}));
}

TEST_F(ScriptorTest, SkipEmptyLocation) {
  Scenario s2({}, MakeScript(1, 77, 177, 300), 2);
  EXPECT_CALL(archive, GetScenario(2)).WillRepeatedly(Return(&s2));

  auto it = scriptor.Load(2, 1);
  EXPECT_EQ(it.ScenarioNumber(), 2);
  EXPECT_EQ(Traverse(it), (std::vector{1, 77, 177, 300}));
}

TEST_F(ScriptorTest, LoadEntrypoint) {
  Scenario s2({}, MakeScript(1, 77, 177, 300), 2);
  s2.script.entrypoints_ = std::map<int, unsigned long>{{1, 77}, {2, 300}};
  EXPECT_CALL(archive, GetScenario(2)).WillRepeatedly(Return(&s2));

  EXPECT_EQ(Traverse(scriptor.LoadEntry(2, 1)), (std::vector{77, 177, 300}));
  EXPECT_EQ(Traverse(scriptor.LoadEntry(2, 2)), (std::vector{300}));
}

TEST_F(ScriptorTest, InvalidLoad) {
  Scenario s({}, MakeScript(1, 10), 100);
  EXPECT_CALL(archive, GetScenario(100)).WillRepeatedly(Return(&s));

  EXPECT_THROW(scriptor.Load(100, 2), std::invalid_argument);
  EXPECT_THROW(scriptor.LoadEntry(100, 1), std::invalid_argument);
}

TEST_F(ScriptorTest, CloneIterator) {
  Scenario s3({}, MakeScript(1, 2, 10, 20, 30, 40), 3);
  EXPECT_CALL(archive, GetScenario(3)).WillRepeatedly(Return(&s3));

  auto it1 = scriptor.Load(3, 2);
  auto it2 = it1++;
  EXPECT_EQ(Traverse(it1), (std::vector{10, 20, 30, 40}));
  EXPECT_EQ(Traverse(it2), (std::vector{2, 10, 20, 30, 40}));
}

TEST_F(ScriptorTest, MultipleScenario) {
  Scenario s3({}, MakeScript(1, 2, 10), 3);
  Scenario s4({}, MakeScript(100, 110, 120), 4);
  EXPECT_CALL(archive, GetScenario(3)).WillRepeatedly(Return(&s3));
  EXPECT_CALL(archive, GetScenario(4)).WillRepeatedly(Return(&s4));

  auto it1 = scriptor.Load(3, 1);
  EXPECT_EQ((*it1++)->GetEntrypoint(), 1);
  auto it2 = scriptor.Load(4, 100);
  EXPECT_EQ(Traverse(it2), (std::vector{100, 110, 120}));
  EXPECT_EQ(Traverse(it1), (std::vector{2, 10}));
}

TEST_F(ScriptorTest, LoadBegin) {
  Scenario s3({}, MakeScript(1, 2, 10), 3);
  EXPECT_CALL(archive, GetScenario(3)).WillRepeatedly(Return(&s3));
  EXPECT_EQ(Traverse(scriptor.Load(3)), (std::vector{1, 2, 10}));
}
