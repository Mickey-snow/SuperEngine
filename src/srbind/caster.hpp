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

#include "srbind/common.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace srbind {

using serilang::Value;

// -------------------------------------------------------------
// type_caster<T>
// -------------------------------------------------------------
template <class T, class Enable = void>
struct type_caster;

// primitives
template <>
struct type_caster<int> {
  static int load(Value& v) {
    if (auto p = v.Get_if<int>())
      return *p;
    throw type_error("expected int, got " + v.Desc());
  }
  static serilang::TempValue cast(int x) { return Value(x); }
};

template <>
struct type_caster<double> {
  static double load(Value& v) {
    if (auto p = v.Get_if<double>())
      return *p;
    if (auto pi = v.Get_if<int>())
      return static_cast<double>(*pi);
    throw type_error("expected double, got " + v.Desc());
  }
  static serilang::TempValue cast(double x) { return Value(x); }
};

template <>
struct type_caster<bool> {
  static bool load(Value& v) {
    if (auto p = v.Get_if<bool>())
      return *p;
    throw type_error("expected bool, got " + v.Desc());
  }
  static serilang::TempValue cast(bool x) { return Value(x); }
};

template <>
struct type_caster<std::string> {
  static std::string load(Value& v) {
    if (auto p = v.Get_if<std::string>())
      return *p;
    throw type_error("expected str, got " + v.Desc());
  }
  static serilang::TempValue cast(std::string s) { return Value(std::move(s)); }
};

template <>
struct type_caster<char const*> {
  static char const* load(Value& v) {
    if (auto p = v.Get_if<std::string>())
      return p->c_str();
    throw type_error("expected str, got " + v.Desc());
  }
  static serilang::TempValue cast(char const* s) {
    return Value(std::string(s));
  }
};

// pass-through Value
template <>
struct type_caster<Value> {
  static Value& load(Value& v) { return v; }
  static serilang::TempValue cast(const Value& v) { return v; }
};

// pointer to bound native instance foreign
template <class T>
struct type_caster<T*, std::enable_if_t<!std::is_void_v<T>>> {
  static T* load(Value& v) {
    if (auto ni = v.Get_if<serilang::NativeInstance>()) {
      T* p = ni->GetForeign<T>();
      if (!p)
        throw type_error("null native instance for requested type");
      return p;
    }
    throw type_error("expected native instance for pointer type, got " +
                     v.Desc());
  }
  // By default, we don't auto-wrap raw T* back into a NativeInstance
  // (doing so requires class registry). Encourage returning
  // Value/void/primitives.
};

// vararg and kwarg types to avoid compile errors
template <>
struct type_caster<std::vector<Value>> {
  static std::vector<Value> load(Value& v) {
    throw type_error("cannot convert to vector");
  }
  static serilang::TempValue cast(const std::vector<Value>&) {
    throw type_error("cannot cast to vector");
  }
};
template <>
struct type_caster<std::unordered_map<std::string, Value>> {
  static std::unordered_map<std::string, Value> load(Value& v) {
    throw type_error("cannot convert to map");
  }
  static serilang::TempValue cast(
      const std::unordered_map<std::string, Value>&) {
    throw type_error("cannot cast to map");
  }
};

// -------------------------------------------------------------

template <class T>
T from_value(Value& v) {
  return type_caster<T>::load(v);
}

template <class R>
serilang::TempValue to_value(R&& r) {
  return type_caster<std::decay_t<R>>::cast(std::forward<R>(r));
}
inline serilang::Value to_value(void) { return serilang::nil; }

}  // namespace srbind
