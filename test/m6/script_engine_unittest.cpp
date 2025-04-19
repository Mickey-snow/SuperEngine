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
#include "m6/script_engine.hpp"
#include "machine/rlmachine.hpp"
#include "utilities/string_utilities.hpp"

namespace m6test {
using namespace m6;

class ScriptEngineTest : public ::testing::Test {
 protected:
  ScriptEngineTest()
      : machine(std::make_shared<RLMachine>(nullptr, nullptr, nullptr)),
        compiler(std::make_shared<Compiler>()),
        interpreter(compiler, machine) {}

  std::shared_ptr<RLMachine> machine;
  std::shared_ptr<Compiler> compiler;
  ScriptEngine interpreter;

  template <typename T>
  inline static std::string Describe(const T& container) {
    return Join(", ", std::views::all(container) |
                          std::views::transform(
                              [](Value const& x) { return x.Desc(); }));
  }
};

TEST_F(ScriptEngineTest, IntermediateValues) {
  auto result = interpreter.Execute(R"(
a=1; 1+1; a+2;
)");
  EXPECT_EQ(Describe(result.intermediateValues), "<int: 2>, <int: 3>");
  EXPECT_TRUE(machine->stack_.empty()) << Describe(machine->stack_);
}

}  // namespace m6test
