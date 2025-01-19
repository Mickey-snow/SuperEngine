// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include "debugger.hpp"

#include "core/expr_ast.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/tokenizer.hpp"
#include "utilities/string_utilities.hpp"

#include <iostream>

Debugger::Debugger(RLMachine& machine) : machine_(machine) {}

constexpr std::string_view copyright_info = R"(
Copyright (C) 2025 Serina Sakurai

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
)";

void Debugger::NotifyBefore(Instruction& instruction) {
  if (!should_break_)
    return;

  static bool should_display_info = true;
  if (should_display_info) {
    should_display_info = false;
    std::cout << copyright_info << std::endl << std::endl;
  }

  while (true) {
    std::cout << "rldbg>" << std::flush;
    std::string input;
    std::getline(std::cin, input);
    trim(input);

    if (input == "q" || input == "quit") {
      should_break_ = false;
      break;
    }

    if (input == "c" || input == "continue")
      break;

    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    std::cout << expr->Apply(Evaluator()) << std::endl;
  }
}
