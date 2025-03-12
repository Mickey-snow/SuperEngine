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
#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <variant>

class Value;
class RLMachine;

using Value_ptr = std::shared_ptr<Value>;

enum class ObjType : uint8_t { Nil, Int, Str, Native };

class IObject {
 public:
  virtual ~IObject() = default;
  virtual ObjType Type() const noexcept = 0;

  virtual std::string Str() const;
  virtual std::string Desc() const;
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

  ObjType Type() const;

  std::any Get() const;
  void* Getptr();

  Value Operator(Op op, Value rhs);
  Value Operator(Op op);

  // for testing
  operator std::string() const;
  bool operator==(int rhs) const;
  bool operator==(const std::string& rhs) const;

 private:
  value_t val_;
};

// should be deprecated soon
Value_ptr make_value(int value);
Value_ptr make_value(std::string value);
Value_ptr make_value(char const* value);
Value_ptr make_value(bool value);
