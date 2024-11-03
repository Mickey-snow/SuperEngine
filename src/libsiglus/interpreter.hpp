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

#ifndef SRC_LIBSIGLUS_INTERPRETER_HPP_
#define SRC_LIBSIGLUS_INTERPRETER_HPP_

#include "libsiglus/ilexeme.hpp"
#include "libsiglus/stack.hpp"

namespace libsiglus {

namespace lex {
class Push;
class Pop;
}  // namespace lex

class Interpreter {
 public:
  Interpreter() = default;

  void Interpret(Lexeme);

  int GetLinenum() const { return lineno_; }
  Stack const& GetStack() const { return stk_; }

 private:
  int lineno_;
  Stack stk_;

  void DispatchPush(std::shared_ptr<lex::Push>);
};

}  // namespace libsiglus

#endif
