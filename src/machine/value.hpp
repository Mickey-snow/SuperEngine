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

  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) {
    return std::visit(std::forward<Visitor>(vis), val_);
  }
  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) const {
    return std::visit(std::forward<Visitor>(vis), val_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) {
    return std::visit<R>(std::forward<Visitor>(vis), val_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) const {
    return std::visit<R>(std::forward<Visitor>(vis), val_);
  }
  template <typename T>
  auto Get_if() {
    return std::get_if<T>(&val_);
  }

  // for testing
  operator std::string() const;
  bool operator==(int rhs) const;
  bool operator==(const std::string& rhs) const;

 private:
  value_t val_;
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

Value make_fn_value(std::string name, auto&& fn) {
  using fn_type = std::decay_t<decltype(fn)>;
  // using R = typename function_traits<fn_type>::result_type;
  // using Argt = typename function_traits<fn_type>::argument_types;

  class NativeImpl : public NativeFunction {
   public:
    NativeImpl(std::string name, fn_type fn)
        : NativeFunction(std::move(name)), fn_(fn) {}

    virtual Value Invoke(RLMachine* /*unused*/,
                         std::vector<Value> args) override {
      return Value(std::monostate());
    }

   private:
    fn_type fn_;
  };

  std::shared_ptr<IObject> fn_obj =
      std::make_shared<NativeImpl>(std::move(name), std::forward<fn_type>(fn));
  return Value(std::move(fn_obj));
}

// should be deprecated soon
Value_ptr make_value(int value);
Value_ptr make_value(std::string value);
Value_ptr make_value(char const* value);
Value_ptr make_value(bool value);
