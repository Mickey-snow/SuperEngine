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

#pragma once

#include <string>

namespace m6 {
// -----------------------------------------------------------------------
// Operator
enum class Op : int {
  Unknown = -1,

  Comma,  // ","

  // Arithmetic Operators
  Add,  // "+"
  Sub,  // "-"
  Mul,  // "*"
  Div,  // "/"
  Mod,  // "%"

  // Bitwise Operators
  BitAnd,      // "&"
  BitOr,       // "|"
  BitXor,      // "^"
  ShiftLeft,   // "<<"
  ShiftRight,  // ">>"
  Tilde,       // "~"

  // Compound Assignment Operators
  AddAssign,         // "+="
  SubAssign,         // "-="
  MulAssign,         // "*="
  DivAssign,         // "/="
  ModAssign,         // "%="
  BitAndAssign,      // "&="
  BitOrAssign,       // "|="
  BitXorAssign,      // "^="
  ShiftLeftAssign,   // "<<="
  ShiftRightAssign,  // ">>="

  // Assignment Operator
  Assign,  // "="

  // Comparison Operators
  Equal,         // "=="
  NotEqual,      // "!="
  LessEqual,     // "<="
  Less,          // "<"
  GreaterEqual,  // ">="
  Greater,       // ">"

  // Logical Operators
  LogicalAnd,  // "&&"
  LogicalOr    // "||"
};
std::string ToString(Op op);
Op CreateOp(std::string_view str);

}  // namespace m6
