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

#include "libsiglus/element.hpp"
#include "libsiglus/value.hpp"

#include <string>
#include <variant>
#include <vector>

namespace libsiglus::token {
struct Command {
  int overload_id;
  ElementCode elmcode;
  Element elm;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type;

  Value dst;

  std::string ToDebugString() const;
};

struct Name {
  std::string str;

  std::string ToDebugString() const { return "Name(" + str + ')'; }
};

struct Textout {
  int kidoku;
  std::string str;

  std::string ToDebugString() const;
};

struct GetProperty {
  ElementCode elmcode;
  Element elm;

  Value dst;
  std::string ToDebugString() const;
};

struct Goto {
  int label;
  std::string ToDebugString() const;
};

struct GotoIf {
  int label;
  bool cond;
  Value src;

  std::string ToDebugString() const;
};

struct Label {
  int id;
  std::string ToDebugString() const;
};

struct Operate1 {
  OperatorCode op;
  Value rhs, dst;
  std::string ToDebugString() const;
};

struct Operate2 {
  OperatorCode op;
  Value lhs, rhs, dst;
  std::string ToDebugString() const;
};

struct Assign {
  ElementCode dst_elmcode;
  Element dst;
  Value src;
  std::string ToDebugString() const;
};

struct Duplicate {
  Value src, dst;
  std::string ToDebugString() const;
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
                             Assign,
                             Duplicate>;
inline std::string ToDebugString(const Token_t& stmt) {
  return std::visit(
      [](const auto& v) -> std::string { return v.ToDebugString(); }, stmt);
}
}  // namespace libsiglus::token
