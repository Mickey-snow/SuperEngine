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

#include "m6/value_internal/lvalue.hpp"

#include "m6/op.hpp"
#include "m6/symbol_table.hpp"

namespace m6 {

lValue::lValue(std::shared_ptr<SymbolTable> sym_tab, std::string name)
    : sym_tab_(sym_tab), name_(name) {}

Value lValue::Deref() const { return sym_tab_->Get(name_); }

std::string lValue::Str() const { return Deref()->Str(); }

std::string lValue::Desc() const { return Deref()->Desc(); }

std::type_index lValue::Type() const { return Deref()->Type(); }

std::any lValue::Get() const { return Deref()->Get(); }

Value lValue::Operator(Op op, Value rhs) const {
  if (auto lval = std::dynamic_pointer_cast<lValue>(rhs))
    rhs = lval->Deref();

  // Handle assignment operators
  switch (op) {
    case Op::Assign:
      sym_tab_->Set(name_, rhs);
      return rhs;

    case Op::AddAssign:
    case Op::SubAssign:
    case Op::MulAssign:
    case Op::DivAssign:
    case Op::ModAssign:
    case Op::BitAndAssign:
    case Op::BitOrAssign:
    case Op::BitXorAssign:
    case Op::ShiftLeftAssign:
    case Op::ShiftRightAssign:
    case Op::ShiftUnsignedRightAssign: {
      static const std::unordered_map<Op, Op> op_map{
          {Op::AddAssign, Op::Add},
          {Op::SubAssign, Op::Sub},
          {Op::MulAssign, Op::Mul},
          {Op::DivAssign, Op::Div},
          {Op::ModAssign, Op::Mod},
          {Op::BitAndAssign, Op::BitAnd},
          {Op::BitOrAssign, Op::BitOr},
          {Op::BitXorAssign, Op::BitXor},
          {Op::ShiftLeftAssign, Op::ShiftLeft},
          {Op::ShiftRightAssign, Op::ShiftRight},
          {Op::ShiftUnsignedRightAssign, Op::ShiftUnsignedRight}};
      rhs = Deref()->Operator(op_map.at(op), rhs);
      sym_tab_->Set(name_, rhs);
      return rhs;
    }

    default:
      break;
  }

  return Deref()->Operator(op, rhs);
}

Value lValue::Operator(Op op) const { return Deref()->Operator(op); }

}  // namespace m6
