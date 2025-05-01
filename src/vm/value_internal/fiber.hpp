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

#include "vm/call_frame.hpp"
#include "vm/value.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace serilang {

enum class FiberState { New, Running, Suspended, Dead };

struct Upvalue;

struct Fiber : public IObject {
  std::vector<Value> stack;
  std::vector<CallFrame> frames;
  FiberState state;
  Value last;
  std::vector<std::shared_ptr<Upvalue>> open_upvalues;

  explicit Fiber(size_t reserve = 64);

  Value* local_slot(size_t frame_index, uint8_t slot);
  std::shared_ptr<Upvalue> capture_upvalue(Value* slot);
  void close_upvalues_from(Value* from);

  ObjType Type() const noexcept override;
  std::string Str() const override;
  std::string Desc() const override;
};

}  // namespace serilang
