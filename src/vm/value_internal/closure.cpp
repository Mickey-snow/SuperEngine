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

#include "vm/value_internal/closure.hpp"

#include "vm/value_internal/fiber.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

namespace serilang {

Closure::Closure(std::shared_ptr<Chunk> c) : chunk(c) {}

std::string Closure::Str() const { return "closure"; }

std::string Closure::Desc() const { return "<closure>"; }

void Closure::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  if (nargs != this->nparams)
    throw std::runtime_error(Desc() + ": arity mismatch");

  const auto base = f.stack.size() - nparams - 1 /*fn slot*/;
  f.frames.push_back({this, entry, base});
}

}  // namespace serilang
