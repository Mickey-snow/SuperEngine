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

#include <unordered_map>

namespace serilang {

struct Dict : public IObject {
  static constexpr inline ObjType objtype = ObjType::Dict;

  std::unordered_map<std::string, Value> map;

  explicit Dict(std::unordered_map<std::string, Value> m = {})
      : map(std::move(m)) {}
  constexpr ObjType Type() const noexcept final { return objtype; }

  std::string Str() const override;   // “{a: 1, b: 2}”
  std::string Desc() const override;  // “<dict{2}>”
};

}  // namespace serilang
