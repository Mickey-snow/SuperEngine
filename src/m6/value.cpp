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

#include "m6/value.hpp"
#include "m6/op.hpp"
#include "m6/value_error.hpp"
#include "m6/value_internal/int.hpp"
#include "m6/value_internal/str.hpp"

namespace m6 {
// factory methods
Value make_value(int value) { return std::make_shared<Int>(value); }
Value make_value(std::string value) {
  return std::make_shared<String>(std::move(value));
}

// -----------------------------------------------------------------------
// class IValue

IValue::IValue() = default;

std::string IValue::Str() const { return "???"; }

std::string IValue::Desc() const { return "???"; }

std::any IValue::Get() const { return std::any(); }  // nil

Value IValue::Operator(Op op, Value rhs) const {
  if (op == Op::Comma)
    return rhs;

  throw UndefinedOperator(op, {this->Desc(), rhs->Desc()});
}

Value IValue::Operator(Op op) const {
  throw UndefinedOperator(op, {this->Desc()});
}

}  // namespace m6
