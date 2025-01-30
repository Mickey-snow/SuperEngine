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

#include "m6/expr_ast.hpp"

#include <any>
#include <string>
#include <typeindex>

namespace m6 {

class Value {
 public:
  Value(std::any val);

  std::string Str() const;
  std::string Desc() const;

  std::type_index Type() const noexcept;

  // for testing
  bool operator==(int rhs) const noexcept;
  bool operator==(std::string_view rhs) const noexcept;

  static Value Calculate(const Value& lhs, Op op, const Value& rhs);
  static Value Calculate(Op op, const Value& self);

 private:
  std::any val_;
};

Value make_value(int value);
Value make_value(std::string value);

}  // namespace m6
