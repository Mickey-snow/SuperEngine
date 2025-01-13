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

#include "base/expr_ast.hpp"

#include <format>
#include <unordered_map>

std::string ToString(Op op) {
  static const std::unordered_map<Op, std::string> opstr_table{
      {Op::Comma, ","},
      {Op::Add, "+"},
      {Op::Sub, "-"},
      {Op::Mul, "*"},
      {Op::Div, "/"},
      {Op::Mod, "%"},
      {Op::BitAnd, "&"},
      {Op::BitOr, "|"},
      {Op::BitXor, "^"},
      {Op::ShiftLeft, "<<"},
      {Op::ShiftRight, ">>"},
      {Op::Tilde, "~"},
      {Op::AddAssign, "+="},
      {Op::SubAssign, "-="},
      {Op::MulAssign, "*="},
      {Op::DivAssign, "/="},
      {Op::ModAssign, "%="},
      {Op::BitAndAssign, "&="},
      {Op::BitOrAssign, "|="},
      {Op::BitXorAssign, "^="},
      {Op::ShiftLeftAssign, "<<="},
      {Op::ShiftRightAssign, ">>="},
      {Op::Assign, "="},
      {Op::Equal, "=="},
      {Op::NotEqual, "!="},
      {Op::LessEqual, "<="},
      {Op::Less, "<"},
      {Op::GreaterEqual, ">="},
      {Op::Greater, ">"},
      {Op::LogicalAnd, "&&"},
      {Op::LogicalOr, "||"}};

  if (opstr_table.contains(op))
    return opstr_table.at(op);
  else
    return std::format("<op:{}>", static_cast<int>(op));
}
Op CreateOp(std::string_view str) {
  static const std::unordered_map<std::string_view, Op> strop_table{
      {",", Op::Comma},
      {"+", Op::Add},
      {"-", Op::Sub},
      {"*", Op::Mul},
      {"/", Op::Div},
      {"%", Op::Mod},
      {"&", Op::BitAnd},
      {"|", Op::BitOr},
      {"^", Op::BitXor},
      {"<<", Op::ShiftLeft},
      {">>", Op::ShiftRight},
      {"~", Op::Tilde},
      {"+=", Op::AddAssign},
      {"-=", Op::SubAssign},
      {"*=", Op::MulAssign},
      {"/=", Op::DivAssign},
      {"%=", Op::ModAssign},
      {"&=", Op::BitAndAssign},
      {"|=", Op::BitOrAssign},
      {"^=", Op::BitXorAssign},
      {"<<=", Op::ShiftLeftAssign},
      {">>=", Op::ShiftRightAssign},
      {"=", Op::Assign},
      {"==", Op::Equal},
      {"!=", Op::NotEqual},
      {"<=", Op::LessEqual},
      {"<", Op::Less},
      {">=", Op::GreaterEqual},
      {">", Op::Greater},
      {"&&", Op::LogicalAnd},
      {"||", Op::LogicalOr}};

  if (strop_table.contains(str))
    return strop_table.at(str);
  else
    return Op::Unknown;
}

std::string BinaryExpr::DebugString() const {
  return lhs->DebugString() + ToString(op) + rhs->DebugString();
}

std::string UnaryExpr::DebugString() const {
  return ToString(op) + sub->DebugString();
}

std::string ParenExpr::DebugString() const {
  return '(' + sub->DebugString() + ')';
}

std::string ReferenceExpr::DebugString() const {
  return id + idx->DebugString();
}

std::string ExprAST::DebugString() const {
  return this->Apply([](const auto& x) -> std::string {
    using T = std::decay_t<decltype(x)>;

    if constexpr (std::same_as<T, std::monostate>)
      return "<null>";
    else if constexpr (std::same_as<T, int>)
      return std::to_string(x);
    else if constexpr (std::same_as<T, std::string>)
      return x;
    else
      return x.DebugString();
  });
}
