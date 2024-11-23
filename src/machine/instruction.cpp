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

#include "libreallive/elements/bytecode.hpp"
#include "libreallive/elements/comma.hpp"
#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/elements/meta.hpp"
#include "libreallive/elements/textout.hpp"
#include "libreallive/expression.hpp"

rlExpression::rlExpression(libreallive::ExpressionElement const* e)
    : expr_(e) {}
int rlExpression::Execute(RLMachine& machine) {
  return expr_->ParsedExpression()->GetIntegerValue(machine);
}

Instruction Resolve(std::shared_ptr<libreallive::BytecodeElement> bytecode) {
  struct Visitor {
    Instruction operator()(libreallive::CommandElement const* cmd) {
      return rlCommand(cmd);
    }
    Instruction operator()(libreallive::CommaElement const*) {
      return std::monostate();
    }
    Instruction operator()(libreallive::MetaElement const* m) {
      switch (m->type_) {
        case libreallive::MetaElement::Line_:
          return Line(m->value_);
        case libreallive::MetaElement::Kidoku_:
          return Kidoku(m->value_);
        default:
          return std::monostate();
      }
    }
    Instruction operator()(libreallive::ExpressionElement const* e) {
      return rlExpression(e);
    }
    Instruction operator()(libreallive::TextoutElement const* e) {
      // Seen files are terminated with the string "SeenEnd", which isn't NULL
      // terminated and has a bunch of random garbage after it.
      constexpr std::string_view SeenEnd{
          "\x82\x72"  // S
          "\x82\x85"  // e
          "\x82\x85"  // e
          "\x82\x8e"  // n
          "\x82\x64"  // E
          "\x82\x8e"  // n
          "\x82\x84"  // d
      };

      std::string unparsed_text = e->GetText();
      if (unparsed_text.starts_with(SeenEnd))
        return End(std::move(unparsed_text));
      else
        return Textout(std::move(unparsed_text));
    }
  };

  return std::visit(Visitor(), bytecode->DownCast());
}
