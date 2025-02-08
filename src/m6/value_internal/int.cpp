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

#include "m6/value_internal/int.hpp"

#include "m6/op.hpp"
#include "m6/exception.hpp"

#include <bit>

namespace m6 {

Int::Int(int val) : val_(val) {}

std::string Int::Str() const { return std::to_string(val_); }

std::string Int::Desc() const { return "<int: " + Str() + '>'; }

std::type_index Int::Type() const noexcept { return typeid(int); }

std::any Int::Get() const { return std::make_any<int>(val_); }

Value Int::Operator(Op op, Value rhs) const {
  try {
    const int rhs_val = std::any_cast<int>(rhs->Get());

    static const auto True = make_value(1);
    static const auto False = make_value(0);

    switch (op) {
      case Op::Comma:
        return rhs;

      case Op::Add:
        return make_value(val_ + rhs_val);
      case Op::Sub:
        return make_value(val_ - rhs_val);
      case Op::Mul:
        return make_value(val_ * rhs_val);
      case Op::Div: {
        if (rhs_val == 0)
          return make_value(0);
        return make_value(val_ / rhs_val);
      }
      case Op::Mod:
        return make_value(val_ % rhs_val);
      case Op::BitAnd:
        return make_value(val_ & rhs_val);
      case Op::BitOr:
        return make_value(val_ | rhs_val);
      case Op::BitXor:
        return make_value(val_ ^ rhs_val);
      case Op::ShiftLeft: {
        if (rhs_val < 0)
          throw ValueError("negative shift count: " + std::to_string(rhs_val));
        return make_value(val_ << rhs_val);
      }
      case Op::ShiftRight: {
        if (rhs_val < 0)
          throw ValueError("negative shift count: " + std::to_string(rhs_val));
        return make_value(val_ >> rhs_val);
      }
      case Op::ShiftUnsignedRight: {
        if (rhs_val < 0)
          throw ValueError("negative shift count: " + std::to_string(rhs_val));
        return make_value(std::bit_cast<uint32_t>(val_) >> rhs_val);
      }
      case Op::Equal:
        return val_ == rhs_val ? True : False;
      case Op::NotEqual:
        return val_ != rhs_val ? True : False;
      case Op::LessEqual:
        return val_ <= rhs_val ? True : False;
      case Op::Less:
        return val_ < rhs_val ? True : False;
      case Op::GreaterEqual:
        return val_ >= rhs_val ? True : False;
      case Op::Greater:
        return val_ > rhs_val ? True : False;

      case Op::LogicalAnd:
        return val_ && rhs_val ? True : False;
      case Op::LogicalOr:
        return val_ || rhs_val ? True : False;

      default:
        break;
    }
  } catch (std::bad_any_cast& e) {
  }

  throw UndefinedOperator(op, {this->Desc(), rhs->Desc()});
}

Value Int::Operator(Op op) const {
  switch (op) {
    case Op::Add:
      return make_value(val_);
    case Op::Sub:
      return make_value(-val_);
    case Op::Tilde:
      return make_value(~val_);

    default:
      break;
  }

  throw UndefinedOperator(op, {this->Desc()});
}

}  // namespace m6
