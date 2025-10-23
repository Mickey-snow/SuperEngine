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

#include "vm/iobject.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace serilang {
struct Dict;
struct Code;

struct Function : public IObject {
  static constexpr inline ObjType objtype = ObjType::Function;

  Dict* globals = nullptr;
  Code* chunk;
  uint32_t entry;

  uint32_t nparam;
  std::unordered_map<std::string, size_t> param_index;
  std::unordered_map<size_t, Value> defaults;

  bool has_vararg;
  bool has_kwarg;

  explicit Function(Code* in_chunk,
                    uint32_t in_entry = 0,
                    uint32_t in_nparam = 0);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) override;
};

}  // namespace serilang
