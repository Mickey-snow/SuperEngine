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

#include "libsiglus/assembler.hpp"

#include "libsiglus/lexeme.hpp"

#include <stdexcept>

namespace libsiglus {

Instruction Assembler::Interpret(Lexeme lex) { return std::visit(*this, lex); }

Instruction Assembler::operator()(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(push.value_);
      break;
    case Type::String: {
      const auto str_val = (*str_table_)[push.value_];
      stack_.Push(str_val);
      break;
    }

    default:
      throw std::runtime_error(
          "Assembler: Unknow type id " +
          std::to_string(static_cast<uint32_t>(push.type_)));
  }
  return std::monostate();
}

Instruction Assembler::operator()(lex::Line line) {
  lineno_ = line.linenum_;
  return std::monostate();
}

Instruction Assembler::operator()(lex::Marker marker) {
  stack_.PushMarker();
  return std::monostate();
}

Instruction Assembler::operator()(lex::Command command) {
  Command result;
  result.return_type = command.rettype_;
  result.overload_id = command.alist_;

  result.named_arg.resize(command.arg_tag_.size());
  result.arg.resize(command.arg_.size() - command.arg_tag_.size());
  for (auto it = result.named_arg.rbegin(); it != result.named_arg.rend();
       ++it) {
    it->first = command.arg_tag_.back();
    it->second = stack_.Pop(command.arg_.back());
    command.arg_tag_.pop_back();
    command.arg_.pop_back();
  }
  for (auto it = result.arg.rbegin(); it != result.arg.rend(); ++it) {
    *it = stack_.Pop(command.arg_.back());
    command.arg_.pop_back();
  }

  result.elm = stack_.Popelm();

  return result;
}

}  // namespace libsiglus
