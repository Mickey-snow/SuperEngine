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

#include "m6/disassembler.hpp"

#include <iomanip>
#include <sstream>

namespace m6 {

std::string Disassemble(const std::vector<Instruction>& instructions) {
  std::ostringstream oss;
  auto digits = std::to_string(instructions.size() + 1).length();

  for (size_t i = 0; i < instructions.size(); ++i) {
    oss << std::setw(digits) << i;
    oss << "│ ";
    oss << std::visit(InstructionToString(), instructions[i]);
    if (i + 1 < instructions.size())
      oss << '\n';
  }

  return oss.str();
}

}  // namespace m6
