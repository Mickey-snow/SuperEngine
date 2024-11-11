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

Instruction Assembler::Interpret(Lexeme lex) {
  return std::visit(*this, lex);
}

Instruction Assembler::operator()(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(push.value_);
      break;

    default:
      throw std::runtime_error(
          "Interpreter: Unknow type id " +
          std::to_string(static_cast<uint32_t>(push.type_)));
  }
  return std::monostate();
}

  Instruction Assembler::operator()(lex::Line line){
    lineno_ = line.linenum_;
    return std::monostate();
  }

  Instruction Assembler::operator()(lex::Marker marker){
    stack_.PushMarker();
    return std::monostate();
  }

}  // namespace libsiglus
