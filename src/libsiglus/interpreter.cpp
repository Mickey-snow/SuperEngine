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
  switch (static_cast<lex::LexType>(lex->GetType())) {
    case lex::LexType::Line: {
      auto line = std::dynamic_pointer_cast<lex::Line>(lex);
      lineno_ = line->linenum_;
      break;
    }

    case lex::LexType::Marker:
      stk_.PushMarker();
      break;

    case lex::LexType::Push: {
      auto push = std::dynamic_pointer_cast<lex::Push>(lex);
      DispatchPush(push);
      break;
    }

    default:
      throw std::runtime_error("Interpreter: Unknown lexeme type " +
                               std::to_string(lex->GetType()));
  }
}

void Interpreter::DispatchPush(std::shared_ptr<lex::Push> push) {
  switch (push->type_) {
    case Type::Int:
      stk_.Push(push->value_);
      break;

    default:
      throw std::runtime_error(
          "Interpreter: Unknow type id " +
          std::to_string(static_cast<uint32_t>(push->type_)));
  }
}

}  // namespace libsiglus
