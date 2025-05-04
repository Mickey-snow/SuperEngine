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
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <iostream>
#include <sstream>

namespace m6test {

using namespace m6;
using namespace serilang;

// Holds everything we care about after one script run
struct ExecutionResult {
  serilang::Value last;   ///< value left on the VM stack (result)
  std::string stdoutStr;  ///< text produced on std::cout
  std::string stderrStr;  ///< text produced on std::cerr
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
      r.stderrStr += pipe.FormatErrors();
      return r;
    }

    auto chunk = pipe.Get();
    if (!chunk) {
      r.stderrStr += "internal: pipeline returned null chunk\n";
      return r;
    }

    // ── run ────────────────────────────────────────────────────────
    try {
      serilang::VM vm(chunk, outBuf);
      vm.Run();
      r.last = vm.main_fiber->last;
    } catch (std::exception const& ex) {
      r.stderrStr += ex.what();
    }

    r.stdoutStr += outBuf.str();
    return r;
  }
};

TEST_F(CompilerTest, ConstantArithmetic) {
  auto res = Run("print(1 + 2);");
  ASSERT_TRUE(res.stderrStr.empty()) << res.stderrStr;
  EXPECT_EQ(trim_cp(res.stdoutStr), "3");
}

TEST_F(CompilerTest, GlobalVariable) {
  auto res = Run("a = 10;\n print(a + 5);");
  ASSERT_TRUE(res.stderrStr.empty()) << res.stderrStr;
  EXPECT_EQ(trim_cp(res.stdoutStr), "15");
}

TEST_F(CompilerTest, MultipleStatements) {
  auto res = Run("x = 4;\n y = 6;\n print(x * y);");
  ASSERT_TRUE(res.stderrStr.empty()) << res.stderrStr;
  EXPECT_EQ(trim_cp(res.stdoutStr), "24");
}

}  // namespace m6test
