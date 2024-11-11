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

#include "libsiglus/interpreter.hpp"
#include "libsiglus/lexeme.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

using namespace libsiglus;

class InterpreterTest : public ::testing::Test {
 protected:
  Interpreter itp;
};

TEST_F(InterpreterTest, Line) {
  const int lineno = 123;
  itp.Interpret(lex::Line(lineno));

  EXPECT_EQ(itp.lineno_, lineno);
}

TEST_F(InterpreterTest, Element) {
  const ElementCode elm{0x3f, 0x4f};
  itp.Interpret(lex::Marker());
  for (const auto& it : elm)
    itp.Interpret(lex::Push(Type::Int, it));

  EXPECT_EQ(itp.stack_.Backelm(), elm);
}
