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
  Assembler assm;
};

TEST_F(AssemblerTest, Line) {
  const int lineno = 123;
  assm.Assemble(lex::Line(lineno));

  EXPECT_EQ(assm.lineno_, lineno);
}

TEST_F(AssemblerTest, Element) {
  const ElementCode elm{0x3f, 0x4f};
  assm.Assemble(lex::Marker());
  for (const auto& it : elm)
    assm.Assemble(lex::Push(Type::Int, it));

  EXPECT_EQ(assm.stack_.Backelm(), elm);
}

TEST_F(AssemblerTest, Command) {
  {
    const ElementCode elm{0x3f, 0x4f};
    assm.Assemble(lex::Marker());
    for (const auto& it : elm)
      assm.Assemble(lex::Push(Type::Int, it));

    auto result = assm.Assemble(lex::Command(0, {}, {}, libsiglus::Type::Int));
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ(std::visit(DebugStringOf(), result), "cmd<63,79:0>() -> int");
  }

  {
    std::vector<std::string> string_table{"ef00", "ef01", "ef02", "ef03"};
    assm.str_table_ = &string_table;

    const ElementCode elm{37, 2, -1, 2, 93, -1, 33, 93, -1, 0, 120};
    assm.Assemble(lex::Marker());
    for (const auto& it : elm)
      assm.Assemble(lex::Push(Type::Int, it));
    assm.Assemble(lex::Push(Type::String, 2));
    for (auto& it : ElementCode{0, 5, 10})
      assm.Assemble(lex::Push(Type::Int, it));

    auto result = assm.Assemble(
        lex::Command(2,
                     {libsiglus::Type::String, libsiglus::Type::Int,
                      libsiglus::Type::Int, libsiglus::Type::Int},
                     {2}, libsiglus::Type::None));
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ(std::visit(DebugStringOf(), result),
              "cmd<37,2,-1,2,93,-1,33,93,-1,0,120:2>(str:ef02,int:0,int:5,_2="
              "int:10) -> typeid:0");
  }
}

TEST_F(AssemblerTest, BinaryOp) {
  struct TestData {
    int lhs;
    OperatorCode op;
    int rhs;
    int expected;
  };
  std::vector<TestData> testdata{
      {1, OperatorCode::Plus, 1, 2},
      {5, OperatorCode::Minus, 10, -5},
      {3, OperatorCode::Mult, 3, 9},
      {10, OperatorCode::Div, 3, 3},
      {10, OperatorCode::Mod, 3, 1},

      {123, OperatorCode::And, 321, 123 & 321},
      {4567, OperatorCode::Or, 312, 4567 | 312},
      {13, OperatorCode::Xor, 41, 13 ^ 41},
      {10, OperatorCode::Sl, 3, 10 << 3},
      {874356, OperatorCode::Sr, 5, 874356 >> 5},

      {1, OperatorCode::LogicalAnd, 4, 1},
      {0, OperatorCode::LogicalAnd, 1, 0},
      {0, OperatorCode::LogicalOr, 1, 1},
      {0, OperatorCode::LogicalOr, 0, 0},
      {31, OperatorCode::Equal, 31, 1},
      {32, OperatorCode::Ne, 31, 1},
  };

  for (const auto& data : testdata) {
    assm.stack_.Clear();
    assm.stack_.Push(data.lhs);
    assm.stack_.Push(data.rhs);
    assm.Assemble(lex::Operate2(Type::Int, Type::Int, data.op));
    EXPECT_EQ(assm.stack_.Backint(), data.expected)
        << "expected " << data.lhs << ToString(data.op) << data.rhs << '='
        << data.expected;
  }
}

TEST_F(AssemblerTest, BinaryOpSpecialCase) {
  {
    assm.stack_.Push("hello ");
    assm.stack_.Push(3);
    assm.Assemble(lex::Operate2(Type::String, Type::Int, OperatorCode::Mult));
    EXPECT_EQ(assm.stack_.Popstr(), "hello hello hello ");
  }

  {
    assm.stack_.Push("hello ");
    assm.stack_.Push("world.");
    assm.Assemble(
        lex::Operate2(Type::String, Type::String, OperatorCode::Plus));
    EXPECT_EQ(assm.stack_.Popstr(), "hello world.");
  }

  {
    assm.stack_.Push("asm");
    assm.stack_.Push("aSm");
    assm.Assemble(lex::Operate2(Type::String, Type::String, OperatorCode::Ne));
    EXPECT_EQ(assm.stack_.Popint(), 0);

    assm.stack_.Push("aBc");
    assm.stack_.Push("abcd");
    assm.Assemble(lex::Operate2(Type::String, Type::String, OperatorCode::Le));
    EXPECT_EQ(assm.stack_.Popint(), 1);
  }

  {
    assm.stack_.Push(0);
    assm.stack_.Push(0);
    assm.Assemble(lex::Operate2(Type::Int, Type::Int, OperatorCode::Div));
    EXPECT_EQ(assm.stack_.Popint(), 0);
    assm.stack_.Push(0);
    assm.stack_.Push(0);
    assm.Assemble(lex::Operate2(Type::Int, Type::Int, OperatorCode::Mod));
    EXPECT_EQ(assm.stack_.Popint(), 0);
  }
}

TEST_F(AssemblerTest, UnaryOp) {
  {
    assm.stack_.Push(123);
    assm.Assemble(lex::Operate1(Type::Int, OperatorCode::Plus));
    EXPECT_EQ(assm.stack_.Popint(), 123);
  }

  {
    assm.stack_.Push(123);
    assm.Assemble(lex::Operate1(Type::Int, OperatorCode::Minus));
    EXPECT_EQ(assm.stack_.Popint(), -123);
  }

  {
    assm.stack_.Push(123);
    assm.Assemble(lex::Operate1(Type::Int, OperatorCode::Inv));
    EXPECT_EQ(assm.stack_.Popint(), (~123));
  }
}
