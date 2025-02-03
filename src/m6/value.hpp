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

using Value = std::shared_ptr<IValue>;
class IValue {
 public:
  IValue();
  virtual ~IValue() = default;

  virtual std::string Str() const;
  virtual std::string Desc() const;

  virtual std::type_index Type() const = 0;

  virtual std::any Get() const;

  virtual Value Operator(Op op, Value rhs) const;
  virtual Value Operator(Op op) const;
};

Value make_value(int value);
Value make_value(std::string value);

}  // namespace m6
