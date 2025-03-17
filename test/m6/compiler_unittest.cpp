// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "util.hpp"

#include "m6/compiler.hpp"
#include "m6/exception.hpp"
#include "m6/native.hpp"
#include "m6/parser.hpp"
#include "machine/op.hpp"
#include "machine/rlmachine.hpp"
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

namespace m6test {
using namespace m6;

class CompilerTest : public ::testing::Test {
 protected:
  CompilerTest()
      : machine(std::make_shared<RLMachine>(nullptr, nullptr, nullptr)),
        stack(const_cast<std::vector<Value>&>(machine->GetStack())) {}

  void Execute(const std::string_view input) {
    auto tok = TokenArray(input);
    auto expr = ParseExpression(std::span(tok));
    auto instructions = compiler.Compile(expr);
    for (const auto& it : instructions)
      std::visit(*machine, it);
  }

  inline std::string DescribeStack() const {
    return Join(", ", std::views::all(machine->GetStack()) |
                          std::views::transform(
                              [](Value const& x) { return x.Desc(); }));
  }

  std::shared_ptr<RLMachine> machine;
  std::vector<Value>& stack;
  Compiler compiler;
};

TEST_F(CompilerTest, GlobalVariable) {
  Execute(R"( beverage = "espresso" )");
  Execute(R"( two = 1 + 1 )");
  EXPECT_EQ(DescribeStack(), "<str: espresso>, <int: 2>");
}

TEST_F(CompilerTest, Assignment) {
  Execute(R"( v2 = 89 )");
  Execute(R"( v3 = "hello" )");
  Execute(R"( v3 = v3 + ", world" )");
  EXPECT_EQ(DescribeStack(), "<int: 89>, <str: hello, world>");
}

TEST_F(CompilerTest, NativeFn) {
  compiler.AddNative(
      make_fn_value("foo", [](int val) { return val == 89 ? 1 : -100; }));
  Execute(R"( v2 = 89 )");
  Execute(R"( foo(v2) )");
  EXPECT_EQ(DescribeStack(), "<int: 89>, <int: 1>");

  EXPECT_THROW(Execute(R"( foo(v2, v2) )"), SyntaxError);
  EXPECT_EQ(DescribeStack(), "<int: 89>, <int: 1>, <nil>");
}

}  // namespace m6test
