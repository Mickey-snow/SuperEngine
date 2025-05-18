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

#include "machine/op.hpp"

#include <memory>
#include <variant>

namespace serilang {

class Fiber;

class Value;

enum class ObjType : uint8_t {
  Nil,
  Bool,
  Int,
  Double,
  Str,
  List,
  Dict,
  Native,
  Closure,
  Fiber,
  Class,
  Instance
};

class IObject;

namespace {
// Trait to detect std::shared_ptr<T>
template <typename>
struct is_shared_ptr : std::false_type {};
template <typename U>
struct is_shared_ptr<std::shared_ptr<U>> : std::true_type {};
}  // namespace

// -----------------------------------------------------------------------
// Value system
class Value {
 public:
  using value_t = std::variant<std::monostate,  // nil
                               bool,
                               int,
                               double,
                               std::string,
                               std::shared_ptr<IObject>  // object
                               >;

  Value(value_t = std::monostate());

  std::string Str() const;
  std::string Desc() const;
  bool IsTruthy() const;

  ObjType Type() const;

  Value Operator(Op op, Value rhs);
  Value Operator(Op op);

  void Call(Fiber& f, uint8_t nargs, uint8_t nkwargs);
  Value Item(const Value& idx);
  Value SetItem(const Value& idx, Value value);
  Value Member(std::string_view mem);
  Value SetMember(std::string_view mem, Value value);

  // Get_if for raw types and IObject-derived pointers
  template <typename T>
  auto Get_if() {
    if constexpr (std::derived_from<T, IObject>) {
      auto basePtr = std::get_if<std::shared_ptr<IObject>>(&val_);
      if (!basePtr || !*basePtr)
        return static_cast<T*>(nullptr);
      auto casted = std::dynamic_pointer_cast<T>(*basePtr);
      return casted ? casted.get() : static_cast<T*>(nullptr);
    } else if constexpr (is_shared_ptr<T>::value) {
      using U = typename T::element_type;
      auto basePtr = std::get_if<std::shared_ptr<IObject>>(&val_);
      if (!basePtr || !*basePtr)
        return T{};
      auto casted = std::dynamic_pointer_cast<U>(*basePtr);
      return casted;
    } else {
      return std::get_if<T>(&val_);
    }
  }

  // Get copies the value out (or copies the shared_ptr)
  template <typename T>
  T Get() {
    if constexpr (is_shared_ptr<T>::value) {
      T sp = Get_if<T>();
      if (!sp)
        throw std::bad_variant_access();
      return sp;
    } else {
      auto ptr = Get_if<T>();
      if (!ptr)
        throw std::bad_variant_access();
      return *ptr;
    }
  }

  // Move retrieves the value by moving out of the variant to avoid copies
  template <typename T>
  T Extract() {
    if constexpr (is_shared_ptr<T>::value) {
      using U = typename T::element_type;
      auto basePtr = std::get<std::shared_ptr<IObject>>(std::move(val_));
      return std::dynamic_pointer_cast<U>(std::move(basePtr));
    } else {
      return std::get<T>(std::move(val_));
    }
  }

  // for testing
  operator std::string() const;
  bool operator==(std::monostate) const;
  bool operator==(int rhs) const;
  bool operator==(double rhs) const;
  bool operator==(bool rhs) const;
  bool operator==(const std::string& rhs) const;
  bool operator==(char const* s) const;

  friend std::ostream& operator<<(std::ostream& os, const Value& v) {
    os << v.Desc();
    return os;
  }

 private:
  value_t val_;
};

// -----------------------------------------------------------------------
class IObject {
 public:
  virtual ~IObject() = default;
  virtual ObjType Type() const noexcept = 0;

  virtual std::string Str() const;
  virtual std::string Desc() const;

  virtual void Call(Fiber& f, uint8_t nargs, uint8_t nkwargs);
  virtual Value Item(const Value& idx);
  virtual Value SetItem(const Value& idx, Value value);
  virtual Value Member(std::string_view mem);
  virtual Value SetMember(std::string_view mem, Value value);
};

}  // namespace serilang
