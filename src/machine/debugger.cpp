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

#include "m6/expr_ast.hpp"
#include "m6/parser.hpp"
#include "m6/tokenizer.hpp"
#include "m6/value.hpp"
#include "machine/rlmachine.hpp"
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
GNU General Public License for more details.)";

void Debugger::Execute() {
  if (!should_break_)
    return;
  should_break_ = false;

  static bool should_display_info = true;
  if (should_display_info) {
    should_display_info = false;
    std::cout << copyright_info << std::endl << std::endl;
  }

  while (true) {
    try {
      std::cout << "rldbg>" << std::flush;
      std::string input;
      std::getline(std::cin, input);
      trim(input);

      if (input.empty())
        continue;

      // process interpreter command
      if (input == "c" || input == "continue")
        break;
      if (input == "l" || input == "list") {
        auto location = machine_.Location();
        std::cout << location.DebugString() << ' ';
        auto instruction = machine_.GetScriptor()->ResolveInstruction(location);
        std::cout << std::visit(
                         InstructionToString(&(machine_.module_manager_)),
                         *instruction)
                  << std::endl;

        continue;
      }
      if (input == "n" || input == "next") {
        should_break_ = true;
        break;
      }

      m6::Tokenizer tokenizer(input);
      auto expr = m6::ParseExpression(std::span(tokenizer.parsed_tok_));
      std::cout << expr->Apply(m6::Evaluator()).Str() << std::endl;
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  }
}

void Debugger::OnEvent(std::shared_ptr<Event> event) {
  if (auto ev = std::get_if<KeyDown>(event.get())) {
    if (ev->code == KeyCode::F2)
      should_break_ = true;
  }
}
