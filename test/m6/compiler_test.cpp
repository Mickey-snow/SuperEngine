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

#include "m6/compiler_pipeline.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/disassembler.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <iostream>
#include <sstream>

namespace m6test {

using namespace m6;
using namespace serilang;

// Holds everything we care about after one script run
struct ExecutionResult {
  serilang::Value last;  ///< value left on the VM stack (result)
  std::string stdout;    ///< text produced on std::cout
  std::string stderr;    ///< text produced on std::cerr
  std::string disasm;    ///< human readable disassembly
};

class CompilerTest : public ::testing::Test {
 public:
  // Compile + run `source`.
  [[nodiscard]] ExecutionResult Run(std::string source) {
    std::stringstream outBuf;
    ExecutionResult r;

    // ── compile ────────────────────────────────────────────────────
    m6::CompilerPipeline pipe(false);
    pipe.compile(std::move(source));

    // any compile-time diagnostics?
    if (!pipe.Ok()) {
      r.stderr += pipe.FormatErrors();
      return r;
    }

    auto chunk = pipe.Get();
    if (!chunk) {
      r.stderr += "internal: pipeline returned null chunk\n";
      return r;
    }
    r.disasm = Disassembler().Dump(*chunk);

    // ── run ────────────────────────────────────────────────────────
    try {
      serilang::VM vm(chunk, outBuf);
      vm.Run();
      r.last = vm.main_fiber->last;
    } catch (std::exception const& ex) {
      r.stderr += ex.what();
    }

    r.stdout += outBuf.str();
    return r;
  }
};

TEST_F(CompilerTest, ConstantArithmetic) {
  auto res = Run("print(1 + 2);");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "3\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, GlobalVariable) {
  auto res = Run("a = 10;\n print(a + 5);");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "15\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, MultipleStatements) {
  auto res = Run("x = 4;\n y = 6;\n print(x * y);");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "24\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, If) {
  auto res = Run(R"(
result = "none";
one = 1;
two = 2;
if(one < two){
  if(one+1 < two) result = "first";
  else { result = one + two; }
} else result = "els";
print(result);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "3\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, While) {
  auto res = Run(R"(
sum = 0;
i = 1;
while(i <= 10){ sum+=i; i+=1; }
print(sum);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "55\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, For) {
  auto res = Run(R"(
fact = 1;
for(i=1; i<10; i+=1) fact *= i;
print(fact);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "362880\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, Function) {
  {
    auto res = Run(R"(
fn print_twice(msg) {
  print(msg);
  print(msg);
}
print_twice("hello");
)");

    ASSERT_TRUE(res.stderr.empty()) << res.stderr;
    EXPECT_EQ(res.stdout, "hello\nhello\n") << "\nDisassembly:\n" << res.disasm;
  }

  {
    auto res = Run(R"(
fn return_plus_1(x) { return x+1; }
print(return_plus_1(123));
)");

    ASSERT_TRUE(res.stderr.empty()) << res.stderr;
    EXPECT_EQ(res.stdout, "124\n") << "\nDisassembly:\n" << res.disasm;
  }
}

TEST_F(CompilerTest, RecursiveFunction) {
  auto res = Run(R"(
fn fib(n) {
  if (n < 2) return 1;
  return fib(n-1) + fib(n-2);
}
print(fib(10));
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "89\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, Class) {
  auto res = Run(R"(
class Klass{
  fn foo(){ return 1; }
  fn boo(x,y){ return x+y; }
}

print(Klass);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "<class Klass>\n") << "\nDisassembly:\n" << res.disasm;
}

}  // namespace m6test
