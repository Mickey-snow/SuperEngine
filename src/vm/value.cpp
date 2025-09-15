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

#include "vm/value.hpp"

#include "utilities/string_utilities.hpp"
#include "vm/iobject.hpp"
#include "vm/object.hpp"
#include "vm/primops.hpp"
#include "vm/vm.hpp"

#include <cmath>
#include <format>

namespace serilang {

// -----------------------------------------------------------------------
// class Value

Value::Value(value_t x) : val_(std::move(x)) {}

std::string Value::Str() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "nil";
        else if constexpr (std::same_as<T, bool>)
          return x ? "true" : "false";
        else if constexpr (std::same_as<T, int>)
          return std::to_string(x);
        else if constexpr (std::same_as<T, double>)
          return std::to_string(x);
        else if constexpr (std::same_as<T, std::string>)
          return x;
        else if constexpr (std::same_as<T, IObject*>)
          return x->Str();
      },
      val_);
}

std::string Value::Desc() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "<nil>";
        else if constexpr (std::same_as<T, bool>)
          return x ? "<bool: true>" : "<bool: false>";
        else if constexpr (std::same_as<T, int>)
          return "<int: " + std::to_string(x) + '>';
        else if constexpr (std::same_as<T, double>)
          return "<double: " + std::to_string(x) + '>';
        else if constexpr (std::same_as<T, std::string>)
          return "<str: " + x + '>';
        else if constexpr (std::same_as<T, IObject*>)
          return x->Desc();
      },
      val_);
}

bool Value::IsTruthy() const {
  return std::visit(
      [](const auto& x) -> bool {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return false;
        else if constexpr (std::same_as<T, bool>)
          return x;
        else if constexpr (std::same_as<T, int>)
          return x != 0;
        else if constexpr (std::same_as<T, double>)
          return x != 0.0;
        else if constexpr (std::same_as<T, std::string>)
          return !x.empty();
        else if constexpr (std::same_as<T, IObject*>) {
          if (!x)
            return false;
          if (auto b = x->Bool(); b.has_value())
            return *b;
          return true;
        }
      },
      val_);
}

ObjType Value::Type() const {
  return std::visit(
      [](const auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, bool>)
          return ObjType::Bool;
        else if constexpr (std::same_as<T, int>)
          return ObjType::Int;
        else if constexpr (std::same_as<T, double>)
          return ObjType::Double;
        else if constexpr (std::same_as<T, std::string>)
          return ObjType::Str;
        else if constexpr (std::same_as<T, IObject*>)
          return x->Type();
        else
          return ObjType::Nil;
      },
      val_);
}

TempValue Value::Operator(VM& vm, Fiber& f, Op op, Value rhs) {
  // 1) Primitive fast path
  {
    if (auto out = primops::EvaluateBinary(op, *this, rhs))
      return TempValue(std::move(*out));
  }

  // 2) Native fast hooks
  if (IObject* lhs_obj = this->Get_if<IObject>()) {
    if (auto r = lhs_obj->BinaryOp(vm, f, op, rhs))
      return *std::move(r);
  }
  if (IObject* rhs_obj = rhs.Get_if<IObject>()) {
    if (auto r = rhs_obj->BinaryOp(vm, f, op, *this))
      return *std::move(r);
  }

  // 3) TODO: Script magic methods (__op__ and __rop__)

  throw UndefinedOperator(op, {this->Desc(), rhs.Desc()});
}

TempValue Value::Operator(VM& vm, Fiber& f, Op op) {
  // 1) Primitive fast path
  {
    if (auto out = primops::EvaluateUnary(op, *this))
      return TempValue(std::move(*out));
  }

  // 2) Native fast hooks
  if (IObject* obj = this->Get_if<IObject>()) {
    if (auto r = obj->UnaryOp(vm, f, op))
      return *std::move(r);
  }

  // 3) TODO: Script magic methods (__neg__/__pos__/__invert__)

  throw UndefinedOperator(op, {this->Desc()});
}

void Value::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->Call(vm, f, nargs, nkwargs);

        else
          throw RuntimeError('\'' + Desc() + "' object is not callable.");
      },
      val_);
}

TempValue Value::Member(std::string_view mem) {
  return std::visit(
      [&](auto& x) -> TempValue {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          return x->Member(mem);

        else
          throw RuntimeError('\'' + Desc() + "' object has no member '" +
                             std::string(mem) + '\'');
      },
      val_);
}

void Value::SetMember(std::string_view mem, Value value) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->SetMember(mem, std::move(value));

        else
          throw RuntimeError('\'' + Desc() +
                             "' object does not support member assignment.");
      },
      val_);
}

void Value::GetItem(VM& vm, Fiber& f) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->GetItem(vm, f);

        else
          vm.Error(f, std::format("'{}' object has no item '{}'", Desc(),
                                  f.stack.back().Str()));
      },
      val_);
}

void Value::SetItem(VM& vm, Fiber& f) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->SetItem(vm, f);

        else
          vm.Error(f,
                   std::format("'{}' object does not support item assignment.",
                               Desc()));
      },
      val_);
}

Value::operator std::string() const { return this->Desc(); }

bool Value::operator==(std::monostate) const {
  return std::holds_alternative<std::monostate>(val_);
}

bool Value::operator==(int rhs) const {
  auto ptr = std::get_if<int>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(double rhs) const {
  auto ptr = std::get_if<double>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(bool rhs) const {
  auto ptr = std::get_if<bool>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(const std::string& rhs) const {
  auto ptr = std::get_if<std::string>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(char const* s) const {
  auto ptr = std::get_if<std::string>(&val_);
  return ptr && *ptr == s;
}

}  // namespace serilang
