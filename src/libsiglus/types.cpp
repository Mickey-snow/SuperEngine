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
    default:
      return "unknown";
  }
}

std::string ToString(OperatorCode op) {
  switch (op) {
    case OperatorCode::None:
      return "<none>";
    case OperatorCode::Equal:
      return "==";
    default:
      return "<?>";
  }
}

}  // namespace libsiglus
