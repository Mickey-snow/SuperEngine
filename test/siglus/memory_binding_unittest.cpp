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

namespace libsiglus::binding {

class SiglusMemoryBindingTest : public ::testing::Test {
 protected:
  SiglusRuntime runtime;

  void SetUp() override {
    runtime.vm = std::make_unique<serilang::VM>(m6::VMFactory::Create());

    Context ctx;
    const auto* bind = SiglusBindingRegistry::Find("memory");
    ASSERT_NE(bind, nullptr);
    (*bind)(ctx, runtime);
  }

  serilang::Value Eval(std::string src) {
    return Execute(*runtime.vm, std::move(src));
  }
};

TEST_F(SiglusMemoryBindingTest, IntBanksReadWriteAndGrow) {
  Eval("A[0] = 7;");
  EXPECT_EQ(runtime.memory->Read(IntBank::A, 0), 7);

  Eval("A[2500] = 9;");
  EXPECT_EQ(runtime.memory->Read(IntBank::A, 2500), 9);
}

TEST_F(SiglusMemoryBindingTest, IntBankBitViewsShareBackingStorage) {
  Eval("B[0] = 0; B.b1[3] = 1;");
  EXPECT_EQ(runtime.memory->Read(IntBank::B, 0), 8);
  EXPECT_EQ(runtime.memory->Read(IntMemoryLocation(IntBank::B, 3, 1)), 1);
}

TEST_F(SiglusMemoryBindingTest, StringBanksReadWriteAndNameBanksWork) {
  Eval(R"(S[0] = "x";)");
  Eval(R"(LN[1] = "local";)");
  Eval(R"(GN[1] = "global";)");

  EXPECT_EQ(runtime.memory->Read(StrBank::S, 0), "x");
  EXPECT_EQ(runtime.memory->Read(StrBank::local_name, 1), "local");
  EXPECT_EQ(runtime.memory->Read(StrBank::global_name, 1), "global");
}

TEST_F(SiglusMemoryBindingTest, BankOperationsRemainCallable) {
  Eval("A.resize(3);");
  EXPECT_EQ(runtime.memory->Size(IntBank::A), 3);

  Eval("A.fill(0, 3, 5); A.Set(1, 11);");
  EXPECT_EQ(runtime.memory->Read(IntBank::A, 1), 11);
  EXPECT_EQ(runtime.memory->Read(IntBank::A, 2), 5);
}

TEST_F(SiglusMemoryBindingTest, PushAndPopFrameRestoresStackBanks) {
  Eval(R"(
L[0] = 42;
K[0] = "old";
)");
  EXPECT_EQ(runtime.memory->Read(IntBank::L, 0), 42);
  EXPECT_EQ(runtime.memory->Read(StrBank::K, 0), "old");

  Eval(R"(
__builtin_push_frame([1, 2], ["arg"]);
L[0] = 99;
K[1] = "new";
)");
  EXPECT_EQ(runtime.memory->Read(IntBank::L, 0), 99);
  EXPECT_EQ(runtime.memory->Read(IntBank::L, 1), 2);
  EXPECT_EQ(runtime.memory->Read(StrBank::K, 0), "arg");
  EXPECT_EQ(runtime.memory->Read(StrBank::K, 1), "new");

  Eval("__builtin_pop_frame();");
  EXPECT_EQ(runtime.memory->Read(IntBank::L, 0), 42);
  EXPECT_EQ(runtime.memory->Read(StrBank::K, 0), "old");
}

}  // namespace libsiglus::binding
