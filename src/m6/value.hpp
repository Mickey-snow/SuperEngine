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

#include "utilities/mpl.hpp"

#include <any>
#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <variant>

namespace m6 {

class Value;
using Value_ptr = std::shared_ptr<Value>;

class IObject {
 public:
  virtual ~IObject() = default;
  virtual std::type_index Type() const noexcept = 0;
};

class Value {
 public:
  using value_t = std::variant<std::monostate,  // nil
                               int,
                               std::string,
                               std::shared_ptr<IObject>  // object
                               >;

  Value(value_t = std::monostate());

  std::string Str() const;
  std::string Desc() const;

  std::type_index Type() const;

  Value_ptr Duplicate();

  std::any Get() const;
  void* Getptr();

  Value_ptr Operator(Op op, Value_ptr rhs);
  Value_ptr Operator(Op op);

  Value_ptr Invoke(std::vector<Value_ptr> args);

 private:
  value_t val_;
};

Value_ptr make_value(int value);
Value_ptr make_value(std::string value);
Value_ptr make_value(char const* value);
Value_ptr make_value(bool value);

}  // namespace m6
