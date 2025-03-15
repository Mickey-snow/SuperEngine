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
#include <map>
#include <string>
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

class NativeFunction : public IObject {
 public:
  NativeFunction(std::string name) : name_(std::move(name)) {}

  std::string Name() const { return name_; }
  virtual ObjType Type() const noexcept override { return ObjType::Native; }

  virtual std::string Str() const override { return "<fn " + name_ + '>'; }
  virtual std::string Desc() const override {
    return "<native function '" + name_ + "'>";
  }

  virtual Value Invoke(RLMachine* machine, std::vector<Value> args) = 0;

 private:
  std::string name_;
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

  Value Operator(Op op, Value rhs);
  Value Operator(Op op);

  template <typename T>
  auto Get_if() -> std::add_pointer_t<T> {
    if constexpr (std::derived_from<T, IObject>) {
      auto ptr = std::get_if<std::shared_ptr<IObject>>(&val_);
      if (!ptr || !(*ptr))
        return nullptr;
      else {
        auto obj = std::dynamic_pointer_cast<T>(*ptr);
        return obj ? obj.get() : nullptr;
      }
    } else {
      return std::get_if<T>(&val_);
    }
  }

  // for testing
  operator std::string() const;
  bool operator==(int rhs) const;
  bool operator==(const std::string& rhs) const;

 private:
  value_t val_;
};
