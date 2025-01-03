// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include "machine/instruction.hpp"

#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/expression.hpp"

#include <format>

std::string rlCommand::ToString() const {
  std::string result =
      std::format("<{},{},{}:{}>", cmd->modtype(), cmd->module(), cmd->opcode(),
                  cmd->overload());

  result += "(";
  bool is_first = true;
  for (const auto& it : cmd->GetParsedParameters()) {
    if (!is_first)
      result += ',';
    is_first = false;
    result += it->GetDebugString();
  }
  result += ')';

  return result;
}

rlExpression::rlExpression(libreallive::ExpressionElement const* e)
    : expr_(e) {}

int rlExpression::Execute(RLMachine& machine) {
  return expr_->ParsedExpression()->GetIntegerValue(machine);
}

std::string rlExpression::ToString() const {
  return expr_->GetSourceRepresentation();
}
