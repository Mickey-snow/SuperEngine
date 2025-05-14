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

#pragma once

#include "vm/value.hpp"

#include <vector>

namespace serilang {

struct List : public IObject {
  std::vector<Value> items;

  explicit List(std::vector<Value> xs = {}) : items(std::move(xs)) {}
  ObjType Type() const noexcept override { return ObjType::List; }

  std::string Str() const override;   // “[1, 2, 3]”
  std::string Desc() const override;  // “<list[3]>”
};

inline Value make_list(std::vector<Value> xs = {}) {
  return Value(std::make_shared<List>(std::move(xs)));
}

}  // namespace serilang
