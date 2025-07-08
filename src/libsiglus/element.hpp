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
#include "libsiglus/function.hpp"
#include "libsiglus/value.hpp"
#include "utilities/flat_map.hpp"

#include <functional>
#include <memory>
#include <span>
#include <utility>
#include <variant>
#include <vector>
#include "boost/container/flat_map.hpp"

namespace libsiglus::elm {

struct Root;
struct Node;

struct Usrcmd {
  int scene, entry;
  std::string_view name;
  std::string ToDebugString() const;
  bool operator==(const Usrcmd&) const = default;
};

struct Usrprop {
  int scene, idx;
  std::string_view name;
  std::string ToDebugString() const;
  bool operator==(const Usrprop&) const = default;
};

struct Mem {
  char bank;
  std::string ToDebugString() const;
  bool operator==(const Mem&) const = default;
};

struct Sym {
  std::string name;
  std::string ToDebugString() const;
  bool operator==(const Sym&) const = default;
};

struct Arg {
  int id;
  std::string ToDebugString() const;
  bool operator==(const Arg&) const = default;
};

struct Farcall {
  Value scn_name;             // type is string
  Value zlabel = Integer(0);  // type is int
  std::vector<Value> intargs;
  std::vector<Value> strargs;

  std::string ToDebugString() const;
  bool operator==(const Farcall&) const = default;
};

struct Wait {
  bool interruptable;
  int time_ms;

  std::string ToDebugString() const;
  bool operator==(const Wait&) const = default;
};

struct Root {
  using var_t = std::variant<std::monostate,
                             Usrcmd,
                             Usrprop,
                             Mem,
                             Sym,  // <-- consider push down to node
                             Arg,
                             Farcall,
                             Wait>;
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
    return std::visit(
        [](const auto& x) -> std::string {
          if constexpr (std::same_as<std::decay_t<decltype(x)>, std::monostate>)
            return "";
          else
            return x.ToDebugString();
        },
        var);
  }
  bool operator==(const Root&) const = default;
};

struct Member {
  std::string_view name;
  std::string ToDebugString() const;
  bool operator==(const Member&) const = default;
};

struct Call {
  std::string_view name;
  std::vector<Value> args;
  std::vector<std::pair<int, Value>> kwargs;
  std::string ToDebugString() const;
  bool operator==(const Call&) const = default;
};

struct Subscript {
  std::optional<Value> idx;
  std::string ToDebugString() const;
  bool operator==(const Subscript&) const = default;
};

struct Val {
  Value value;
  std::string ToDebugString() const;
  bool operator==(const Val&) const = default;
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
  bool operator==(const Node&) const = default;
};

struct AccessChain {
  Root root;
  std::vector<Node> nodes;

  std::string ToDebugString() const;
  Type GetType() const;
  bool operator==(const AccessChain&) const = default;
};

}  // namespace libsiglus::elm
