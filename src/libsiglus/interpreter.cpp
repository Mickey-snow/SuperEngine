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

#include "libsiglus/interpreter.hpp"

#include "libsiglus/lexeme.hpp"

#include <stdexcept>

namespace libsiglus {

void Interpreter::Interpret(Lexeme lex) {
  std::visit(
      [&](auto& lex) {
        using T = std::decay_t<decltype(lex)>;

        if constexpr (std::is_same_v<T, lex::Line>) {
          lineno_ = lex.linenum_;
        } else if constexpr (std::is_same_v<T, lex::Marker>) {
          stk_.PushMarker();
        } else if constexpr (std::is_same_v<T, lex::Push>) {
          DispatchPush(lex);
        } else {
          throw std::runtime_error("Interpreter: Unknown lexeme type.");
        }
      },
      lex);
}

void Interpreter::DispatchPush(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stk_.Push(push.value_);
      break;

    default:
      throw std::runtime_error(
          "Interpreter: Unknow type id " +
          std::to_string(static_cast<uint32_t>(push.type_)));
  }
}

}  // namespace libsiglus
