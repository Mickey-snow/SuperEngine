// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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
// -----------------------------------------------------------------------

#pragma once

#include "types.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace libsiglus {

struct Integer {
  std::string ToDebugString() const { return "int:" + std::to_string(val_); }
  auto operator<=>(const Integer&) const = default;

  int val_;
};

struct String {
  std::string ToDebugString() const { return "str:" + val_; }
  auto operator<=>(const String&) const = default;

  std::string val_;
};

struct Variable {
  std::string ToDebugString() const { return 'v' + std::to_string(id); }
  auto operator<=>(const Variable&) const = default;

  Type type;
  int id;
};

// represents a siglus property
using Value = std::variant<Integer,  // constant
                           String,   // constant
                           Variable>;
inline int AsInt(const Value& val) { return std::get<Integer>(val).val_; }
inline std::string AsStr(const Value& val) {
  return std::get_if<String>(&val)->val_;
}
std::optional<Value> TryEval(OperatorCode op, Value rhs);
std::optional<Value> TryEval(Value lhs, OperatorCode op, Value rhs);

inline std::string ToString(const auto& v) {
  return std::visit([](const auto& x) { return x.ToDebugString(); }, v);
}

inline Type Typeof(const Value& v) {
  return std::visit(
      [](const auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, Integer>)
          return Type::Int;
        else if constexpr (std::same_as<T, String>)
          return Type::String;
        else if constexpr (std::same_as<T, Variable>)
          return x.type;
        else
          return Type::None;
      },
      v);
}
}  // namespace libsiglus
