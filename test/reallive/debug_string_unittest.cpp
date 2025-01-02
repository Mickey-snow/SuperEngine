// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/elements/meta.hpp"
#include "libreallive/visitors.hpp"
#include "machine/module_manager.hpp"

#include <string>
#include <utility>

using namespace libreallive;

TEST(DebugStringVisitorTest, Meta) {
  {
    MetaElement meta(MetaElement::Entrypoint_, 10);
    EXPECT_EQ(std::visit(DebugStringVisitor(), meta.DownCast()),
              "#entrypoint 10");
  }

  {
    MetaElement meta(MetaElement::Line_, 20);
    EXPECT_EQ(std::visit(DebugStringVisitor(), meta.DownCast()), "#line 20");
  }

  {
    MetaElement meta(MetaElement::Kidoku_, 30);
    EXPECT_EQ(std::visit(DebugStringVisitor(), meta.DownCast()), "#kidoku 30");
  }
}

TEST(DebugStringVisitorTest, Command) {
  IModuleManager& module_manager = ModuleManager::GetInstance();

  {
    FunctionElement fn(
        CommandInfo{.cmd = {1, 1, 82, 0xe8, 0x03, 0, 0, 0},
                    .param = {ExpressionFactory::IntConstant(1),
                              ExpressionFactory::StrConstant("2")}});
    EXPECT_EQ(std::visit(DebugStringVisitor(), fn.DownCast()),
              "op<1:082:01000, 0>(1, \"2\")");
    EXPECT_EQ(std::visit(DebugStringVisitor(&module_manager), fn.DownCast()),
              "objBgMove(1, \"2\")");
  }

  {
    char repr[] = {0, 0, 1, 0, 0, 0, 0, 0};
    GotoElement jump(repr, 0x123);
    EXPECT_EQ(std::visit(DebugStringVisitor(), jump.DownCast()),
              "op<0:001:00000, 0>() @291");
    EXPECT_EQ(std::visit(DebugStringVisitor(&module_manager), jump.DownCast()),
              "goto() @291");
  }
}

TEST(DebugStringVisitorTest, Expression) {
  {
    ExpressionElement expr(ExpressionFactory::BinaryExpression(
        Op::BitOr,
        ExpressionFactory::MemoryReference(27,
                                           ExpressionFactory::IntConstant(123)),
        ExpressionFactory::MemoryReference(
            107, ExpressionFactory::IntConstant(456))));
    EXPECT_EQ(std::visit(DebugStringVisitor(), expr.DownCast()),
              "intB1b[123] | intD8b[456]");
  }

  {
    Expression num[8];
    for (int i = 0; i < 8; ++i)
      num[i] = ExpressionFactory::MemoryReference(
          1, ExpressionFactory::IntConstant(i));
    ExpressionElement expr(ExpressionFactory::BinaryExpression(
        Op::Add,
        ExpressionFactory::BinaryExpression(
            Op::BitOr,
            ExpressionFactory::BinaryExpression(Op::BitAnd, num[0], num[1]),
            ExpressionFactory::BinaryExpression(Op::BitXor, num[2], num[3])),
        ExpressionFactory::BinaryExpression(
            Op::Div,
            ExpressionFactory::UnaryExpression(
                Op::Sub,
                ExpressionFactory::BinaryExpression(Op::BitOr, num[4], num[5])),
            ExpressionFactory::BinaryExpression(Op::BitAnd, num[6], num[7]))));

    EXPECT_EQ(std::visit(DebugStringVisitor(), expr.DownCast()),
              "intB[0] & intB[1] | intB[2] ^ intB[3] + -intB[4] | intB[5] / "
              "intB[6] & intB[7]");
    // Parentheses are lost during parsing
  }
}
