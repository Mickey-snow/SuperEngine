// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "libsiglus/assembler.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/value.hpp"

#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

using namespace libsiglus;

class AssemblerTest : public ::testing::Test {
 protected:
  Assembler itp;
};

TEST_F(AssemblerTest, Line) {
  const int lineno = 123;
  itp.Interpret(lex::Line(lineno));

  EXPECT_EQ(itp.lineno_, lineno);
}

TEST_F(AssemblerTest, Element) {
  const ElementCode elm{0x3f, 0x4f};
  itp.Interpret(lex::Marker());
  for (const auto& it : elm)
    itp.Interpret(lex::Push(Type::Int, it));

  EXPECT_EQ(itp.stack_.Backelm(), elm);
}

TEST_F(AssemblerTest, Command) {
  {
    const ElementCode elm{0x3f, 0x4f};
    itp.Interpret(lex::Marker());
    for (const auto& it : elm)
      itp.Interpret(lex::Push(Type::Int, it));

    auto result = itp.Interpret(lex::Command(0, {}, {}, libsiglus::Type::Int));
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ(std::visit(DebugStringOf(), result), "cmd<63,79:0>() -> int");
  }

  {
    const ElementCode elm{37, 2, -1, 2, 93, -1, 33, 93, -1, 0, 120};
    itp.Interpret(lex::Marker());
    for (const auto& it : elm)
      itp.Interpret(lex::Push(Type::Int, it));
    itp.Interpret(lex::Push(Type::String, 2));
    for (auto& it : ElementCode{1, 0, 5, 10})
      itp.Interpret(lex::Push(Type::Int, it));
    std::vector<std::string> string_table{"ef00", "ef01", "ef02", "ef03"};

    auto result = itp.Interpret(
        lex::Command(2,
                     {libsiglus::Type::String, libsiglus::Type::Int,
                      libsiglus::Type::Int, libsiglus::Type::Int},
                     {2}, libsiglus::Type::None));
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    // EXPECT_EQ(
    //     std::visit(DebugStringOf(), result),
    //     "cmd<37,2,-1,2,93,-1,33,93,-1,0,120:2>(ef02,1,0,5,_2=10) -> typeid:0");
  }
}
