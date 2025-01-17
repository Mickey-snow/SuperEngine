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

#include "core/expr_ast.hpp"

class ExprASTTest : public ::testing::Test {
 protected:
};

TEST_F(ExprASTTest, DebugPrint) {
  auto base = BinaryExpr(Op::Add, std::make_unique<ExprAST>(1),
                         std::make_unique<ExprAST>(2));
  auto lhs = std::make_unique<ExprAST>(
      ParenExpr(std::make_unique<ExprAST>(std::move(base))));
  auto rhs = std::make_unique<ExprAST>(
      UnaryExpr(Op::Sub, std::make_unique<ExprAST>(3)));

  auto ast = ExprAST(BinaryExpr(Op::Mul, std::move(lhs), std::move(rhs)));
  EXPECT_EQ(ast.DebugString(), "(1+2)*-3");
};
