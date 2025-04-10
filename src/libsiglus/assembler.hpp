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

#include "libsiglus/lexfwd.hpp"
#include "libsiglus/stack.hpp"
#include "libsiglus/value.hpp"
#include "utilities/string_utilities.hpp"

#include <format>
#include <string>
#include <variant>

namespace libsiglus {

struct Command {
  int overload_id;
  std::vector<int> elm;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type;

  std::string ToDebugString() const {
    const std::string cmd_repr = std::format(
        "cmd<{}:{}>", Join(",", elm | std::views::transform([](const auto& x) {
                                  return std::to_string(x);
                                })),
        overload_id);

    std::vector<std::string> args_repr;
    args_repr.reserve(arg.size() + named_arg.size());

    std::transform(
        arg.cbegin(), arg.cend(), std::back_inserter(args_repr),
        [](const Value& value) { return std::visit(DebugStringOf(), value); });
    std::transform(named_arg.cbegin(), named_arg.cend(),
                   std::back_inserter(args_repr), [](const auto& it) {
                     return std::format("_{}={}", it.first,
                                        std::visit(DebugStringOf(), it.second));
                   });

    return std::format("{}({}) -> {}", cmd_repr, Join(",", args_repr),
                       ToString(return_type));
  }
};

struct Name {
  std::string str;

  std::string ToDebugString() const { return "Name(" + str + ')'; }
};

struct Textout {
  int kidoku;
  std::string str;

  std::string ToDebugString() const {
    return std::format("Textout@{} ({})", kidoku, str);
  }
};

using Instruction = std::variant<std::monostate, Command, Name, Textout>;

// this class takes the low-level `Lexeme` and constructs `Instruction` objects
// that are ready for execution.
class Assembler {
 public:
  Assembler() = default;

  Instruction Assemble(Lexeme);

 public:
  // dispatch functions
  Instruction operator()(lex::Push);
  Instruction operator()(lex::Line);
  Instruction operator()(lex::Marker);
  Instruction operator()(lex::Operate1);
  Instruction operator()(lex::Operate2);
  Instruction operator()(lex::Copy);
  Instruction operator()(lex::CopyElm);

  Instruction operator()(lex::Command);
  Instruction operator()(lex::Namae);
  Instruction operator()(lex::Textout);

  template <typename T>
  Instruction operator()(T) {
    throw -1;  // not implemented yet
  }

 public:
  int lineno_;
  Stack stack_;
  std::vector<std::string>* str_table_;
};

}  // namespace libsiglus
