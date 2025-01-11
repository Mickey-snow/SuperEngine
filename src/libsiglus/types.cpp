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

#include "libsiglus/types.hpp"

namespace libsiglus {

std::string ToString(Type type) {
  switch (type) {
    case Type::Int:
      return "int";
    case Type::String:
      return "str";
    case Type::Label:
      return "lebel";
    case Type::List:
      return "list";
    default:
      return "typeid:" + std::to_string(static_cast<uint32_t>(type));
  }
}

std::string ToString(OperatorCode op) {
  switch (op) {
    case OperatorCode::None:
      return "<none>";
    case OperatorCode::Plus:
      return "+";
    case OperatorCode::Minus:
      return "-";
    case OperatorCode::Mult:
      return "*";
    case OperatorCode::Div:
      return "/";
    case OperatorCode::Mod:
      return "%";
    case OperatorCode::Equal:
      return "==";
    case OperatorCode::Ne:
      return "!=";
    case OperatorCode::Gt:
      return ">";
    case OperatorCode::Ge:
      return ">=";
    case OperatorCode::Lt:
      return "<";
    case OperatorCode::Le:
      return "<=";
    case OperatorCode::LogicalAnd:
      return "&&";
    case OperatorCode::LogicalOr:
      return "||";
    case OperatorCode::Inv:
      return "~";
    case OperatorCode::And:
      return "&";
    case OperatorCode::Or:
      return "|";
    case OperatorCode::Xor:
      return "^";
    case OperatorCode::Sl:
      return "<<";
    case OperatorCode::Sr:
      return ">>";
    case OperatorCode::Sru:
      return "u>>";
    default:
      return "<?>";
  }
}

}  // namespace libsiglus
