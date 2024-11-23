// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
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

#include "libreallive/visitors.hpp"

#include "encodings/cp932.hpp"
#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/elements/meta.hpp"
#include "libreallive/elements/textout.hpp"
#include "machine/module_manager.hpp"
#include "utilities/string_utilities.hpp"

#include <format>

namespace libreallive {

DebugStringVisitor::DebugStringVisitor(IModuleManager* manager)
    : manager_(manager) {}

std::string DebugStringVisitor::operator()(MetaElement const* meta) {
  std::string type_str;
  switch (meta->type_) {
    case MetaElement::Entrypoint_:
      type_str = "entrypoint";
      break;
    case MetaElement::Kidoku_:
      type_str = "kidoku";
      break;
    case MetaElement::Line_:
      type_str = "line";
      break;
  }

  return std::format("#{} {}", type_str, meta->value_);
}

std::string DebugStringVisitor::operator()(CommandElement const* cmd) {
  std::string repr;
  try {
    if (manager_)
      repr = manager_->GetCommandName(*cmd);
  } catch (...) {
    repr.clear();
  }

  if (repr.empty())
    repr = std::format("op<{}:{:03}:{:05}, {}>", cmd->modtype(), cmd->module(),
                       cmd->opcode(), cmd->overload());

  repr += '(';
  bool first = true;
  for (const auto& param : cmd->GetParsedParameters()) {
    if (first)
      first = false;
    else
      repr += ", ";
    repr += param->GetDebugString();
  }
  repr += ')';

  std::string tag_repr = cmd->GetTagsRepresentation();
  if (!tag_repr.empty())
    repr += ' ' + tag_repr;

  return repr;
}

std::string DebugStringVisitor::operator()(ExpressionElement const* elm) {
  return elm->GetSourceRepresentation();
}

std::string DebugStringVisitor::operator()(CommaElement const*) {
  return "<comma>";
}

std::string DebugStringVisitor::operator()(TextoutElement const* t) {
  static Cp932 converter;
  return std::format("text({})",
                     UnicodeToUTF8(converter.ConvertString(t->GetText())));
}

}  // namespace libreallive
