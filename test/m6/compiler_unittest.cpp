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

  void Execute(const std::string_view input, bool allow_error = false) {
    try {
      auto tok = TokenArray(input);
      auto begin = tok.data(), end = tok.data() + tok.size();
      while (begin != end && !begin->HoldsAlternative<tok::Eof>()) {
        auto instructions = compiler.Compile(ParseStmt(begin, end));
        machine->halted_ = false;
        machine->ip_ = 0;
        machine->script_ = std::span(instructions);
        machine->Execute();

        while (begin != end && begin->HoldsAlternative<tok::WS>())
          ++begin;
      }
    } catch (CompileError& e) {
      if (allow_error)
        throw;
      ADD_FAILURE() << e.FormatWith(input);
    } catch (std::exception& e) {
      if (allow_error)
        throw;
      ADD_FAILURE() << e.what();
    }
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
  Execute(R"(
beverage = "espresso";
two = 1 + 1;
)");
  EXPECT_EQ(DescribeStack(), "<str: espresso>, <int: 2>");
}

TEST_F(CompilerTest, Assignment) {
  Execute(R"(
v2 = 89;
v3 = "hello";
v3 = v3 + ", world";
)");

  EXPECT_EQ(DescribeStack(), "<int: 89>, <str: hello, world>");
}

TEST_F(CompilerTest, NativeFn) {
  compiler.AddNative(
      make_fn_value("foo", [](int val) { return val == 89 ? 1 : -100; }));

  Execute(R"(
v2 = 89;
foo(v2);
)");
  EXPECT_EQ(DescribeStack(), "<int: 89>, <int: 1>");

  EXPECT_THROW(Execute(R"( foo(v2, v2); )", true), SyntaxError);
  EXPECT_EQ(DescribeStack(), "<int: 89>, <int: 1>, <nil>");
}

TEST_F(CompilerTest, IfStmt) {
  Execute(R"(
a = 10;
b = 20;
result = "";
if (a < b)
  if (a < 5) result += "a is less than 5";
  else result += "a is less than b";
else result += "a is not less than b";
)");
  EXPECT_EQ(DescribeStack(), "<int: 10>, <int: 20>, <str: a is less than b>");

  Execute(R"(
a = 10;
if(a >= 10) a += 10;
a += 10;
)");
  EXPECT_EQ(DescribeStack(), "<int: 30>, <int: 20>, <str: a is less than b>");
}

TEST_F(CompilerTest, WhileStmt) {
  Execute(R"(
i = 1;
while (i < 10) i += i;
i += 1;
)");
  EXPECT_EQ(DescribeStack(), "<int: 17>");
}

TEST_F(CompilerTest, ForStmt) {
  Execute(R"(
sum = 0;
for(i=0;i<12;i+=1) sum -= i;
sum=-sum;
)");
  EXPECT_EQ(DescribeStack(), "<int: 66>, <int: 12>");

  Execute(R"(
for(;i+sum>24;) sum -= 1;
)");
  EXPECT_EQ(DescribeStack(), "<int: 12>, <int: 12>");
}

}  // namespace m6test
