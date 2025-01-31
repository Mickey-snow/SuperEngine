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

#include "m6/op.hpp"

#include <format>
#include <unordered_map>

namespace m6 {

// -----------------------------------------------------------------------
// enum Op helper methods
std::string ToString(Op op) {
  switch (op) {
    case Op::Comma:
      return ",";
    case Op::Add:
      return "+";
    case Op::Sub:
      return "-";
    case Op::Mul:
      return "*";
    case Op::Div:
      return "/";
    case Op::Mod:
      return "%";
    case Op::BitAnd:
      return "&";
    case Op::BitOr:
      return "|";
    case Op::BitXor:
      return "^";
    case Op::ShiftLeft:
      return "<<";
    case Op::ShiftRight:
      return ">>";
    case Op::Tilde:
      return "~";
    case Op::AddAssign:
      return "+=";
    case Op::SubAssign:
      return "-=";
    case Op::MulAssign:
      return "*=";
    case Op::DivAssign:
      return "/=";
    case Op::ModAssign:
      return "%=";
    case Op::BitAndAssign:
      return "&=";
    case Op::BitOrAssign:
      return "|=";
    case Op::BitXorAssign:
      return "^=";
    case Op::ShiftLeftAssign:
      return "<<=";
    case Op::ShiftRightAssign:
      return ">>=";
    case Op::Assign:
      return "=";
    case Op::Equal:
      return "==";
    case Op::NotEqual:
      return "!=";
    case Op::LessEqual:
      return "<=";
    case Op::Less:
      return "<";
    case Op::GreaterEqual:
      return ">=";
    case Op::Greater:
      return ">";
    case Op::LogicalAnd:
      return "&&";
    case Op::LogicalOr:
      return "||";
    default:
      return std::format("<op:{}>", static_cast<int>(op));
  }
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

}  // namespace m6
