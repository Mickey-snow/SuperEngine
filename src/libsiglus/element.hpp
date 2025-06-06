// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#pragma once

#include "libsiglus/element_code.hpp"
#include "libsiglus/value.hpp"
#include "utilities/flat_map.hpp"

#include <functional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace libsiglus::elm {

struct Usrcmd {
  int scene, entry;
  std::string_view name;
  std::string ToDebugString() const;
};

struct Usrprop {
  int scene, idx;
  std::string_view name;
  std::string ToDebugString() const;
};

struct Mem {
  char bank;
  std::string ToDebugString() const;
};

struct Sym {
  std::string name;
  std::string ToDebugString() const;
};

struct Print {  // callable (str)->void
  int kidoku;
  std::string ToDebugString() const;
};

struct Arg {
  int id;
  std::string ToDebugString() const;
};

struct Root {
  using var_t = std::variant<Usrcmd, Usrprop, Mem, Sym, Arg, Print>;
  var_t var;
  Type type;

  template <typename... Ts>
    requires std::constructible_from<var_t, Ts...>
  Root(Type type, Ts&&... params)
      : var(std::forward<Ts>(params)...), type(type) {}
  template <typename... Ts>
    requires std::constructible_from<var_t, Ts...>
  Root(Ts&&... params)
      : var(std::forward<Ts>(params)...), type(Type::Invalid) {}

  inline std::string ToDebugString() const {
    return std::visit([](const auto& x) { return x.ToDebugString(); }, var);
  }
};

struct Member {
  std::string_view name;
  std::string ToDebugString() const;
};

struct Call {
  std::string_view name;
  std::vector<Value> args;
  std::string ToDebugString() const;
};

struct Subscript {
  std::optional<Value> idx;
  std::string ToDebugString() const;
};

struct Val {
  Value value;
  std::string ToDebugString() const;
};

struct Node {
  using var_t = std::variant<Member, Call, Subscript, Val>;
  var_t var;
  Type type;

  template <typename... Ts>
    requires std::constructible_from<var_t, Ts...>
  Node(Type type, Ts&&... params)
      : var(std::forward<Ts>(params)...), type(type) {}
  template <typename... Ts>
    requires std::constructible_from<var_t, Ts...>
  Node(Ts&&... params)
      : var(std::forward<Ts>(params)...), type(Type::Invalid) {}

  inline std::string ToDebugString() const {
    return std::visit([](const auto& x) { return x.ToDebugString(); }, var);
  }
};

struct AccessChain {
  Root root;
  std::vector<Node> nodes;

  std::string ToDebugString() const;
  Type GetType() const;
};

}  // namespace libsiglus::elm
