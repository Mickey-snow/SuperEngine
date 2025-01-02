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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "libreallive/elements/command.hpp"
#include "machine/module_manager.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"

using libreallive::CommandElement;
using libreallive::CommandInfo;
class MockCommandElement : public CommandElement {
 public:
  MockCommandElement(int modtype, int module_id, int opcode, int overload)
      : CommandElement(CommandInfo()),
        modtype_(modtype),
        module_id_(module_id),
        opcode_(opcode),
        overload_(overload) {}
  ~MockCommandElement() override = default;

  int modtype() const override { return modtype_; }
  int module() const override { return module_id_; }
  int opcode() const override { return opcode_; }
  int overload() const override { return overload_; }

  MOCK_METHOD(size_t, GetBytecodeLength, (), (const, override));

 private:
  int modtype_, module_id_, opcode_, overload_;
};

namespace {
class BooModule : public RLModule {
 public:
  static constexpr auto modtype = 12;
  static constexpr auto module_id = 23;

  BooModule() : RLModule("Boo", modtype, module_id) {
    struct Boo1 : public RLOpcode<> {
      void operator()(RLMachine&) {}
    };
    AddOpcode(10, 0, "Boo1", new Boo1);

    AddUnsupportedOpcode(0, 1, "Boo2");
    AddUnsupportedOpcode(1, 1, "Boo3");
  }
};

class FooModule : public RLModule {
 public:
  static constexpr auto modtype = 0;
  static constexpr auto module_id = 0;

  FooModule() : RLModule("Foo", modtype, module_id) {
    struct Foo1 : public RLOpcode<> {
      void operator()(RLMachine&) {}
    };
    AddOpcode(0, 0, "Foo1", new Foo1);

    AddUnsupportedOpcode(0, 1, "Foo2");
    AddUnsupportedOpcode(1, 1, "Foo3");
  }
};
}  // namespace

class ModuleManagerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    auto foo = std::make_unique<FooModule>();
    foo_ptr = foo.get();
    auto boo = std::make_unique<BooModule>();
    boo_ptr = boo.get();
    ModuleManager::AttachModule(std::move(foo));
    ModuleManager::AttachModule(std::move(boo));
  }

  void SetUp() override {
    int modtype = FooModule::modtype, modid = FooModule::module_id;
    foo1_cmd = std::make_shared<MockCommandElement>(modtype, modid, 0, 0);
    foo2_cmd = std::make_shared<MockCommandElement>(modtype, modid, 0, 1);
    foo3_cmd = std::make_shared<MockCommandElement>(modtype, modid, 1, 1);
    modtype = BooModule::modtype, modid = BooModule::module_id;
    boo1_cmd = std::make_shared<MockCommandElement>(modtype, modid, 10, 0);
    boo2_cmd = std::make_shared<MockCommandElement>(modtype, modid, 0, 1);
    boo3_cmd = std::make_shared<MockCommandElement>(modtype, modid, 1, 1);
  }

  inline static IModuleManager& manager = ModuleManager::GetInstance();
  inline static RLModule *foo_ptr, *boo_ptr;
  std::shared_ptr<MockCommandElement> foo1_cmd;
  std::shared_ptr<MockCommandElement> foo2_cmd;
  std::shared_ptr<MockCommandElement> foo3_cmd;
  std::shared_ptr<MockCommandElement> boo1_cmd;
  std::shared_ptr<MockCommandElement> boo2_cmd;
  std::shared_ptr<MockCommandElement> boo3_cmd;
};

TEST_F(ModuleManagerTest, ResolveOperation) {
  RLOperation* op = manager.Dispatch(*foo1_cmd);
  ASSERT_NE(op, nullptr);
  EXPECT_EQ(op->Name(), "Foo1");

  op = manager.Dispatch(*boo2_cmd);
  ASSERT_NE(op, nullptr);
  EXPECT_EQ(op->Name(), "Boo2");
}

TEST_F(ModuleManagerTest, GetCommandName) {
  EXPECT_EQ(manager.GetCommandName(*foo1_cmd), "Foo1");
  EXPECT_EQ(manager.GetCommandName(*foo2_cmd), "Foo2");
  EXPECT_EQ(manager.GetCommandName(*foo3_cmd), "Foo3");
  EXPECT_EQ(manager.GetCommandName(*boo1_cmd), "Boo1");
  EXPECT_EQ(manager.GetCommandName(*boo2_cmd), "Boo2");
  EXPECT_EQ(manager.GetCommandName(*boo3_cmd), "Boo3");
}

TEST_F(ModuleManagerTest, GetCommandNameInvalid) {
  {
    MockCommandElement invalid_cmd(99, 99, 99, 99);
    EXPECT_TRUE(manager.GetCommandName(invalid_cmd).empty());
  }

  {
    MockCommandElement invalid_cmd(FooModule::modtype, FooModule::module_id,
                                   999, 0);
    EXPECT_TRUE(manager.GetCommandName(invalid_cmd).empty());
  }
}

TEST_F(ModuleManagerTest, RejectDoubleRegister) {
  EXPECT_THROW(ModuleManager::AttachModule(std::make_unique<FooModule>()),
               std::invalid_argument);
  EXPECT_THROW(ModuleManager::AttachModule(std::make_unique<BooModule>()),
               std::invalid_argument);
}

TEST_F(ModuleManagerTest, GetModule) {
  auto result = manager.GetModule(FooModule::modtype, FooModule::module_id);
  EXPECT_EQ(result, foo_ptr);

  result = manager.GetModule(BooModule::modtype, BooModule::module_id);
  EXPECT_EQ(result, boo_ptr);

  result = manager.GetModule(999, 999);
  EXPECT_EQ(result, nullptr);
}
