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

#include "vm/value_internal/class.hpp"

namespace serilang {

ObjType Class::Type() const noexcept { return ObjType::Class; }

std::string Class::Str() const { return "class"; }

std::string Class::Desc() const { return "<class>"; }

Instance::Instance(std::shared_ptr<Class> klass_) : klass(std::move(klass_)) {}

ObjType Instance::Type() const noexcept { return ObjType::Instance; }

std::string Instance::Str() const { return "instance"; }

std::string Instance::Desc() const { return "<instance>"; }

}  // namespace serilang
