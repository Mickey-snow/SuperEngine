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
    std::string result = "cmd<";
    bool first = true;
    for (const auto it : elm) {
      if (first)
        first = false;
      else
        result += ',';
      result += std::to_string(it);
    }
    result += ':' + std::to_string(overload_id) + ">(";

    first = true;
    for (const auto& it : arg) {
      if (first)
        first = false;
      else
        result += ',';
      result += std::visit(DebugStringOf(), it);
    }
    for (const auto& [name, it] : named_arg) {
      if (first)
        first = false;
      else
        result += ',';
      result +=
          '_' + std::to_string(name) + '=' + std::visit(DebugStringOf(), it);
    }

    result += ") -> " + ToString(return_type);
    return result;
  }
};

using Instruction = std::variant<std::monostate, Command>;

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
  Instruction operator()(lex::Command);

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
