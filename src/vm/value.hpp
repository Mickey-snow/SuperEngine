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
#include "vm/objtype.hpp"
#include "vm/value_fwd.hpp"

#include <memory>
#include <variant>

namespace serilang {

struct GCVisitor;

class VM;

// -----------------------------------------------------------------------
// Value system
class Value {
 public:
  using value_t = std::variant<std::monostate,  // nil
                               bool,
                               int,
                               double,
                               std::string,
                               IObject*  // object
                               >;

  Value(value_t x = std::monostate());

  std::string Str() const;
  std::string Desc() const;
  bool IsTruthy() const;

  ObjType Type() const;

  TempValue Operator(Op op, Value rhs);
  TempValue Operator(Op op);

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs);
  TempValue Item(const Value& idx);
  TempValue SetItem(const Value& idx, Value value);
  TempValue Member(std::string_view mem);
  TempValue SetMember(std::string_view mem, Value value);

  template <typename T>
  auto Get_if() -> std::add_pointer_t<T> {
    using TPTR = std::add_pointer_t<T>;

    if constexpr (std::same_as<T, IObject>) {
      auto objptr = std::get_if<IObject*>(&val_);
      if (!objptr)
        return static_cast<TPTR>(nullptr);
      return *objptr;
    } else if constexpr (std::derived_from<T, IObject>) {
      auto baseptr = std::get_if<IObject*>(&val_);
      if (!baseptr)
        return static_cast<TPTR>(nullptr);
      auto casted = dynamic_cast<TPTR>(*baseptr);
      return casted;
    } else {
      return std::get_if<T>(&val_);
    }
  }

  template <typename T>
  auto Get() -> T {
    if constexpr (std::is_pointer_v<T> &&
                  std::derived_from<std::remove_pointer_t<T>, IObject>) {
      auto ptr = Get_if<std::remove_pointer_t<T>>();
      if (!ptr)
        throw std::bad_variant_access();
      return ptr;
    } else {
      auto ptr = Get_if<T>();
      if (!ptr)
        throw std::bad_variant_access();
      return *ptr;
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

inline const auto nil = Value(std::monostate());

}  // namespace serilang
