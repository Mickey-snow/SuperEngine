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

#include <cstdint>
#include <string_view>
#include <vector>
#include <variant>

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
  const ElementCode elm{0x3f, 0x4f};
  itp.Interpret(lex::Marker());
  for (const auto& it : elm)
    itp.Interpret(lex::Push(Type::Int, it));

  auto result = itp.Interpret(lex::Command(0, {}, {}, libsiglus::Type::Int));
  ASSERT_TRUE(std::holds_alternative<Command>(result));
  EXPECT_EQ(std::visit(DebugStringOf(), result), "cmd<63,79:0>() -> int");
}
