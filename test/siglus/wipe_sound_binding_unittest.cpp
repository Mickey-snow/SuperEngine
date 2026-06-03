// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/siglus_runtime.hpp"
#include "m6/vm_factory.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <string>
#include <variant>

namespace libsiglus::binding {

class SiglusWipeBindingTest : public ::testing::Test {
 protected:
  SiglusRuntime runtime;

  void SetUp() override {
    runtime.vm = std::make_unique<serilang::VM>(m6::VMFactory::Create());

    Context ctx;
    const auto* bind = SiglusBindingRegistry::Find("wipe");
    ASSERT_NE(bind, nullptr);
    (*bind)(ctx, runtime);
  }

  serilang::Value Eval(std::string src) {
    return Execute(*runtime.vm, std::move(src));
  }

  serilang::Value Global(std::string name) {
    return runtime.vm->globals_->at(std::move(name));
  }
};

TEST_F(SiglusWipeBindingTest, WipePacketFastForwards) {
  Eval(R"(
wipe.wipe(nil, [1, 500, 0, [1, 2]], {"_8": 0, "_3": [7, 6]});
result = wipe.check();
)");
  EXPECT_EQ(Global("result"), 0);

  Eval(R"(
wipe.wipe_mask(nil, ["mask", 1, 2, 3, [4, 5]], {"_5": 99});
result = wipe.wait(nil, [], {"_0": -1});
)");
  EXPECT_EQ(Global("result"), 0);
}

TEST_F(SiglusWipeBindingTest, WipeEndIsCallable) {
  Eval("wipe.wipe(nil, [1], {});");
  EXPECT_TRUE(Eval("wipe.end(nil, [], {});") == std::monostate{});
  Eval("result = wipe.check(nil, [], {});");
  EXPECT_EQ(Global("result"), 0);
}

class SiglusSoundBindingTest : public ::testing::Test {
 protected:
  SiglusRuntime runtime;

  void SetUp() override {
    runtime.vm = std::make_unique<serilang::VM>(m6::VMFactory::Create());

    Context ctx;
    const auto* bind = SiglusBindingRegistry::Find("sound");
    ASSERT_NE(bind, nullptr);
    (*bind)(ctx, runtime);
  }

  serilang::Value Eval(std::string src) {
    return Execute(*runtime.vm, std::move(src));
  }

  serilang::Value Global(std::string name) {
    return runtime.vm->globals_->at(std::move(name));
  }
};

TEST_F(SiglusSoundBindingTest, BgmFadeStateAndWaitKey) {
  Eval("result = bgm.check();");
  EXPECT_EQ(Global("result"), 0);

  Eval("bgm.stop(1800);");
  Eval("result = bgm.check();");
  EXPECT_EQ(Global("result"), 2);
  Eval("result = bgm.wait_fade_key();");
  EXPECT_EQ(Global("result"), 0);
  Eval("result = bgm.check();");
  EXPECT_EQ(Global("result"), 0);
}

TEST_F(SiglusSoundBindingTest, BgmMetadataAndVolume) {
  Eval(R"(bgm.ready("song01"); result = bgm.get_regist_name();)");
  EXPECT_EQ(Global("result"), "song01");
  Eval("bgm.set_volume(123); result = bgm.get_volume();");
  EXPECT_EQ(Global("result"), 123);
  Eval("result = bgm.get_play_pos();");
  EXPECT_EQ(Global("result"), 0);
}

class SiglusSystemBindingTest : public ::testing::Test {
 protected:
  SiglusRuntime runtime;

  void SetUp() override {
    runtime.vm = std::make_unique<serilang::VM>(m6::VMFactory::Create());

    Context ctx;
    const auto* bind = SiglusBindingRegistry::Find("system");
    ASSERT_NE(bind, nullptr);
    (*bind)(ctx, runtime);
  }

  serilang::Value Eval(std::string src) {
    return Execute(*runtime.vm, std::move(src));
  }

  serilang::Value Global(std::string name) {
    return runtime.vm->globals_->at(std::move(name));
  }
};

TEST_F(SiglusSystemBindingTest, WaitGlobalsAreCallable) {
  Eval("wait(10); result = wait_key(10);");
  EXPECT_EQ(Global("result"), 0);
}

}  // namespace libsiglus::binding
