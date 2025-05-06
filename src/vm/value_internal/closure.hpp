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

#include <cstdint>
#include <memory>
#include <vector>

namespace serilang {

struct Chunk;
struct Upvalue;

struct Closure : public IObject {
  std::shared_ptr<Chunk> chunk;
  uint32_t entry{};
  uint32_t nparams{};
  uint32_t nlocals{};
  std::vector<std::shared_ptr<Upvalue>> up;

  explicit Closure(std::shared_ptr<Chunk> c);

  ObjType Type() const noexcept override;
  std::string Str() const override;
  std::string Desc() const override;
};

}  // namespace serilang
