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

#include "libsiglus/element.hpp"

#include "utilities/string_utilities.hpp"

#include <format>

namespace libsiglus {

// -----------------------------------------------------------------------
// class IElement
IElement::IElement(Type type_) : type(type_) {}

std::string IElement::ToDebugString() const {
  return std::format("elm<{}>", ToString(type));
}

// -----------------------------------------------------------------------
// class UserCommand
std::string UserCommand::ToDebugString() const {
  return std::format("{}<{}:{}>", name, scene, entry);
}
elm::Kind UserCommand::Kind() const noexcept { return elm::Kind::Usrcmd; }

// -----------------------------------------------------------------------
// class UserProperty
std::string UserProperty::ToDebugString() const {
  return std::format("{}<{}:{}>", name, scene, idx);
}
elm::Kind UserProperty::Kind() const noexcept { return elm::Kind::Usrprop; }

// -----------------------------------------------------------------------
// class UnknownElement
std::string UnknownElement::ToDebugString() const {
  return "???" +
         std::format("<{}>",
                     Join(",", std::views::all(elmcode.code) |
                                   std::views::transform([](const Value& v) {
                                     return ToString(v);
                                   })));
}
elm::Kind UnknownElement::Kind() const noexcept { return elm::Kind::Invalid; }

}  // namespace libsiglus
