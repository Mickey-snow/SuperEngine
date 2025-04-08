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
#include "m6/disassembler.hpp"
#include "m6/exception.hpp"
#include "m6/native.hpp"
#include "m6/script_engine.hpp"
#include "machine/rlmachine.hpp"
#include "utilities/string_utilities.hpp"

namespace m6test {
using namespace m6;

class CompilerTest : public ::testing::Test {
 protected:
  CompilerTest()
      : machine(std::make_shared<RLMachine>(nullptr, nullptr, nullptr)),
        compiler(std::make_shared<Compiler>()),
        interpreter(compiler, machine) {}

  inline std::string DescribeStack() const {
    return Join(", ", std::views::all(machine->stack_) |
                          std::views::transform(
                              [](Value const& x) { return x.Desc(); }));
  }

  inline std::string DescribeGlobals() const {
    return Join(", ",
                std::views::all(machine->globals_) |
                    std::views::transform([](std::optional<Value> const& x) {
                      return x.has_value() ? x->Desc() : "<null>";
                    }));
  }

  std::shared_ptr<RLMachine> machine;
  std::shared_ptr<Compiler> compiler;
  ScriptEngine interpreter;
};

TEST_F(CompilerTest, Expression) {
  auto ins = interpreter.Execute(R"(1+1;)").instructions;
  EXPECT_TRUE(machine->stack_.empty()) << Disassemble(ins);
  EXPECT_TRUE(machine->globals_.empty()) << Disassemble(ins);
}

TEST_F(CompilerTest, GlobalVariable) {
  interpreter.Execute(R"(
beverage = "espresso";
two = 1 + 1;
)");
  EXPECT_EQ(DescribeGlobals(), "<str: espresso>, <int: 2>");
}

TEST_F(CompilerTest, Assignment) {
  interpreter.Execute(R"(
v2 = 89;
v3 = "hello";
v3 = v3 + ", world";
)");

  EXPECT_EQ(DescribeGlobals(), "<int: 89>, <str: hello, world>");
}

TEST_F(CompilerTest, NativeFn) {
  compiler->AddNative(
      make_fn_value("foo", [](int val) { return val == 89 ? 1 : -100; }));

  interpreter.Execute(R"(
v2 = 89;
v3 = foo(v2);
)");
  EXPECT_EQ(DescribeGlobals(), "<int: 89>, <int: 1>");

  auto result = interpreter.Execute(R"( v4 = foo(v2, v2); )");
  EXPECT_TXTEQ(interpreter.FlushErrors(), "Too many arguments provided.");

  EXPECT_EQ(DescribeGlobals(), "<int: 89>, <int: 1>");
}

TEST_F(CompilerTest, IfStmt) {
  interpreter.Execute(R"(
a = 10;
b = 20;
result = "";
if (a < b) {
  if (a < 5) result += "a is less than 5";
  else result += "a is less than b";
}
else result += "a is not less than b";
)");
  EXPECT_EQ(DescribeGlobals(), "<int: 10>, <int: 20>, <str: a is less than b>");

  interpreter.Execute(R"(
a = 10;
if(a >= 10){ a += 10; }
a += 10;
)");
  EXPECT_EQ(DescribeGlobals(), "<int: 30>, <int: 20>, <str: a is less than b>");
}

TEST_F(CompilerTest, WhileStmt) {
  interpreter.Execute(R"(
i = 1;
sum = 0;
while (i < 10){ sum += i; i += 1; }
)");
  EXPECT_EQ(DescribeGlobals(), "<int: 10>, <int: 45>");
}

TEST_F(CompilerTest, ForStmt1) {
  auto ins = interpreter
                 .Execute(R"(
sum = 0;
for(i=0;i<12;i+=1) sum -= i;
sum=-sum;
)")
                 .instructions;
  EXPECT_EQ(DescribeGlobals(), "<int: 66>") << Disassemble(ins);
}

TEST_F(CompilerTest, ForStmt2) {
  auto ins = interpreter
                 .Execute(R"(
rows = 5;
result = "";
for(i=1;i<=rows;i+=1){
  for(j=1;j<=i;j+=1)
    result += "*";
  result += "\n";
}
)")
                 .instructions;

  EXPECT_EQ(DescribeGlobals(), "<int: 5>, <str: *\n**\n***\n****\n*****\n>")
      << Disassemble(ins);
}

}  // namespace m6test
