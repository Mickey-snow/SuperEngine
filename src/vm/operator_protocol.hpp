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

#include "machine/op.hpp"

#include <string_view>

namespace serilang::opproto {

inline std::string_view UnaryMagic(Op op) {
  switch (op) {
    case Op::Add:
      return "__pos__";
    case Op::Sub:
      return "__neg__";
    case Op::Tilde:
      return "__invert__";
    default:
      return {};
  }
}

struct BinaryMagicNames {
  std::string_view lhs;      // __op__
  std::string_view rhs;      // __rop__
  std::string_view inplace;  // __iop__
};

inline BinaryMagicNames BinaryMagic(Op op) {
  switch (op) {
    case Op::Add:
      return {"__add__", "__radd__", "__iadd__"};
    case Op::Sub:
      return {"__sub__", "__rsub__", "__isub__"};
    case Op::Mul:
      return {"__mul__", "__rmul__", "__imul__"};
    case Op::Div:
      return {"__truediv__", "__rtruediv__", "__itruediv__"};
    case Op::Mod:
      return {"__mod__", "__rmod__", "__imod__"};
    case Op::Pow:
      return {"__pow__", "__rpow__", "__ipow__"};
    case Op::BitAnd:
      return {"__and__", "__rand__", "__iand__"};
    case Op::BitOr:
      return {"__or__", "__ror__", "__ior__"};
    case Op::BitXor:
      return {"__xor__", "__rxor__", "__ixor__"};
    case Op::ShiftLeft:
      return {"__lshift__", "__rlshift__", "__ilshift__"};
    case Op::ShiftRight:
      return {"__rshift__", "__rrshift__", "__irshift__"};
    case Op::ShiftUnsignedRight:
      return {"__urshift__", "__rurshift__", "__iurshift__"};
    case Op::Equal:
      return {"__eq__", "__req__", {}};
    case Op::NotEqual:
      return {"__ne__", "__rne__", {}};
    case Op::Less:
      return {"__lt__", "__rlt__", {}};
    case Op::LessEqual:
      return {"__le__", "__rle__", {}};
    case Op::Greater:
      return {"__gt__", "__rgt__", {}};
    case Op::GreaterEqual:
      return {"__ge__", "__rge__", {}};
    default:
      return {};
  }
}

}  // namespace serilang::opproto
