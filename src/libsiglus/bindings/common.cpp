// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "libsiglus/bindings/common.hpp"

#include "vm/exception.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/value.hpp"

#include <format>

namespace libsiglus ::binding {

namespace sr = serilang;

std::optional<int> AsInt(const sr::Value& value) {
  if (const int* int_value = value.Get_if<int>())
    return *int_value;
  if (const bool* bool_value = value.Get_if<bool>())
    return *bool_value ? 1 : 0;
  if (const double* double_value = value.Get_if<double>())
    return static_cast<int>(*double_value);
  return std::nullopt;
}

std::string AsString(const sr::Value& value) {
  if (const sr::String* str = value.Get_if<sr::String>())
    return str->str_;
  return value.Str();
}

int RequireInt(sr::Value const& value, std::string_view where) {
  if (auto* i = value.Get_if<int>())
    return *i;
  throw sr::RuntimeError(
      std::format("expected int for {}, got {}", where, value.Desc()));
}

const sr::List* RequireList(sr::Value const& value, std::string_view where) {
  if (auto* list = value.Get_if<sr::List>())
    return list;
  throw sr::RuntimeError(
      std::format("expected list for {}, got {}", where, value.Desc()));
}

}  // namespace libsiglus::binding
