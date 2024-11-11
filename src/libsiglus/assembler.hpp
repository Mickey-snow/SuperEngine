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

#ifndef SRC_LIBSIGLUS_ASSEMBLER_HPP_
#define SRC_LIBSIGLUS_ASSEMBLER_HPP_

#include "libsiglus/lexfwd.hpp"
#include "libsiglus/stack.hpp"

#include <variant>

namespace libsiglus {

using Instruction = std::variant<std::monostate>;

class Assembler {
 public:
  Assembler() = default;

  Instruction Interpret(Lexeme);

 public:
  // dispatch functions
  Instruction operator()(lex::Push);
  Instruction operator()(lex::Line);
  Instruction operator()(lex::Marker);

  template <typename T>
  Instruction operator()(T) {
    throw -1;  // not implemented yet
  }

 public:
  int lineno_;
  Stack stack_;
};

}  // namespace libsiglus

#endif
