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
#include "machine/module_manager.hpp"
#include "utilities/string_utilities.hpp"

#include <format>

rlExpression::rlExpression(libreallive::ExpressionElement const* e)
    : expr_(e) {}

int rlExpression::Execute(RLMachine& machine) {
  return expr_->ParsedExpression()->GetIntegerValue(machine);
}

// -----------------------------------------------------------------------
// struct InstructionToString
InstructionToString::InstructionToString(ModuleManager const* manager)
    : manager_(manager) {}

std::string InstructionToString::operator()(std::monostate) const {
  return "<null>";
}
std::string InstructionToString::operator()(const Kidoku& p) const {
  return "kidoku " + std::to_string(p.num);
}
std::string InstructionToString::operator()(const Line& p) const {
  return "line " + std::to_string(p.num);
}
std::string InstructionToString::operator()(const rlCommand& p) const {
  std::string result =
      manager_ == nullptr ? "???" : manager_->GetCommandName(*p.cmd);  // name

  result += std::format("<{},{},{}:{}>", p.cmd->modtype(), p.cmd->module(),
                        p.cmd->opcode(), p.cmd->overload());  // opcode

  result += '(' +
            Join(",", std::views::all(p.cmd->GetParsedParameters()) |
                          std::views::transform([](libreallive::Expression x) {
                            return x->GetDebugString();
                          })) +
            ')';  // arguments

  return result;
}
std::string InstructionToString::operator()(const rlExpression& p) const {
  return p.expr_->GetSourceRepresentation();
}
std::string InstructionToString::operator()(const Textout& p) const {
  return "text: " + p.text;
}
std::string InstructionToString::operator()(const Push& p) const {
  return "push " + p.value.Desc();
}
std::string InstructionToString::operator()(const Pop& p) const {
  if (p.count == 1)
    return "pop";
  else
    return "pop " + std::to_string(p.count);
}
std::string InstructionToString::operator()(const BinaryOp& p) const {
  return "op2 " + ToString(p.op);
}
std::string InstructionToString::operator()(const UnaryOp& p) const {
  return "op1 " + ToString(p.op);
}
std::string InstructionToString::operator()(const Load& p) const {
  return "ld " + std::to_string(p.offset);
}
std::string InstructionToString::operator()(const Store& p) const {
  return "st " + std::to_string(p.offset);
}
std::string InstructionToString::operator()(const End& p) const {
  return "<end>";
}
