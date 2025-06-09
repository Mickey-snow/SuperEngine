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

#include "libsiglus/argument_list.hpp"
#include "libsiglus/element.hpp"
#include "libsiglus/element_code.hpp"
#include "libsiglus/value.hpp"

#include <string>
#include <variant>
#include <vector>

namespace libsiglus::token {
struct Command {
  ElementCode elmcode;
  int overload_id;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type;

  elm::AccessChain chain;
  Value dst;

  std::string ToDebugString() const;
  bool operator==(const Command&) const = default;
};

struct Name {
  Value str;

  std::string ToDebugString() const;
  bool operator==(const Name&) const = default;
};

struct Textout {
  int kidoku;
  Value str;

  std::string ToDebugString() const;
  bool operator==(const Textout&) const = default;
};

struct GetProperty {
  ElementCode elmcode;

  elm::AccessChain chain;
  Value dst;
  std::string ToDebugString() const;
  bool operator==(const GetProperty&) const = default;
};

struct Goto {
  int label;
  std::string ToDebugString() const;
  bool operator==(const Goto&) const = default;
};

struct GotoIf {
  int label;
  bool cond;

  Value src;

  std::string ToDebugString() const;
  bool operator==(const GotoIf&) const = default;
};

struct Label {
  int id;
  std::string ToDebugString() const;
  bool operator==(const Label&) const = default;
};

struct Operate1 {
  OperatorCode op;
  Value rhs, dst;
  std::optional<Value> val;  // if can be eval at compile time
  std::string ToDebugString() const;
  bool operator==(const Operate1&) const = default;
};

struct Operate2 {
  OperatorCode op;
  Value lhs, rhs, dst;
  std::optional<Value> val;  // if can be eval at compile time
  std::string ToDebugString() const;
  bool operator==(const Operate2&) const = default;
};

struct Assign {
  ElementCode dst_elmcode;
  elm::AccessChain dst;
  Value src;
  std::string ToDebugString() const;
  bool operator==(const Assign&) const = default;
};

struct Duplicate {
  Value src, dst;
  std::string ToDebugString() const;
  bool operator==(const Duplicate&) const = default;
};

struct Gosub {
  int entry_id;
  std::vector<Value> args;
  Value dst;
  std::string ToDebugString() const;
  bool operator==(const Gosub&) const = default;
};

struct Subroutine {
  std::string name;
  std::vector<Type> args;
  std::string ToDebugString() const;
  bool operator==(const Subroutine&) const = default;
};

struct Return {
  std::vector<Value> ret_vals;
  std::string ToDebugString() const;
  bool operator==(const Return&) const = default;
};

struct Eof {
  std::string ToDebugString() const;
  bool operator<=>(const Eof&) const = default;
};

using Token_t = std::variant<Command,
                             Name,
                             Textout,
                             GetProperty,
                             Operate1,
                             Operate2,
                             Label,
                             Goto,
                             GotoIf,
                             Gosub,
                             Assign,
                             Duplicate,
                             Subroutine,
                             Return,
                             Eof>;

inline std::string ToString(const Token_t& stmt) {
  return std::visit(
      [](const auto& v) -> std::string { return v.ToDebugString(); }, stmt);
}

}  // namespace libsiglus::token
