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

#include "m6/value_internal/str.hpp"

#include "m6/exception.hpp"
#include "m6/op.hpp"

namespace m6 {
String::String(std::string val) : val_(std::move(val)) {}

std::string String::Str() const { return val_; }
std::string String::Desc() const { return "<str: " + val_ + '>'; }

std::type_index String::Type() const noexcept { return typeid(std::string); }

std::any String::Get() const { return val_; }

Value String::Duplicate() { return make_value(val_); }

Value String::Operator(Op op, Value rhs) {
  if (rhs->Type() == typeid(int)) {
    auto rhs_value = std::any_cast<int>(rhs->Get());
    if (rhs_value >= 0 && (op == Op::Mul || op == Op::MulAssign)) {
      std::string result, current = val_;
      result.reserve(val_.size() * rhs_value);
      while (rhs_value) {
        if (rhs_value & 1)
          result += current;
        current = current + current;
        rhs_value >>= 1;
      }

      if (op == Op::MulAssign)
        val_ = result;
      return make_value(std::move(result));
    }
  }

  else if (rhs->Type() == typeid(std::string)) {
    const auto rhs_value = std::any_cast<std::string>(rhs->Get());
    static const auto True = make_value(1);
    static const auto False = make_value(0);

    switch (op) {
      case Op::Equal:
        return val_ == rhs_value ? True : False;
      case Op::NotEqual:
        return val_ != rhs_value ? True : False;
      case Op::Add:
        return make_value(val_ + rhs_value);
      case Op::AddAssign:
        val_ += rhs_value;
        return make_value(val_);
      default:
        break;
    }
  }

  throw UndefinedOperator(op, {this->Desc(), rhs->Desc()});
}

Value String::Operator(Op op) { throw UndefinedOperator(op, {this->Desc()}); }

}  // namespace m6
