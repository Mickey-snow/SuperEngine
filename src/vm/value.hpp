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

#include "machine/value.hpp"
#include "vm/instruction.hpp"

namespace serilang {

struct Chunk {
  std::vector<Instruction> code;
  std::vector<Value> const_pool;
  [[nodiscard]] const Instruction& operator[](size_t i) const {
    return code[i];
  }
};

struct Upvalue {
  Value* location;  // points into a Fiber stack slot while open
  Value closed;     // heap copy once closed
  bool is_closed{false};
  Value get() const { return is_closed ? closed : *location; }
  void set(Value v) {
    if (is_closed)
      closed = v;
    else
      *location = v;
  }
};

struct Closure : public IObject {
  std::shared_ptr<Chunk> chunk;
  uint32_t entry{};
  uint32_t nparams{}, nlocals{};
  std::vector<std::shared_ptr<Upvalue>> up;  // fixed size nupvals
  explicit Closure(std::shared_ptr<Chunk> c) : chunk(std::move(c)) {}

  ObjType Type() const noexcept override { return ObjType::Closure; }
  std::string Str() const override { return "closure"; }
  std::string Desc() const override { return "<closure>"; }
};

struct Class : public IObject {
  std::string name;
  std::unordered_map<std::string, Value> methods;

  ObjType Type() const noexcept override { return ObjType::Class; }
  std::string Str() const override { return "class"; }
  std::string Desc() const override { return "<class>"; }
};
struct Instance : public IObject {
  std::shared_ptr<Class> klass;
  std::unordered_map<std::string, Value> fields;

  Instance(std::shared_ptr<Class> klass_) : klass(klass_) {}

  ObjType Type() const noexcept override { return ObjType::Instance; }
  std::string Str() const override { return "instance"; }
  std::string Desc() const override { return "<instance>"; }
};

struct CallFrame {
  std::shared_ptr<Closure> closure;
  uint32_t ip;  // index into chunk->code
  size_t bp;    // base pointer into fiber stack
};
enum class FiberState { New, Running, Suspended, Dead };

struct Fiber : public IObject {
  std::vector<Value> stack;
  std::vector<CallFrame> frames;
  FiberState state = FiberState::New;
  Value last;  // return / yielded / thrown
  std::vector<std::shared_ptr<Upvalue>> open_upvalues;

  explicit Fiber(size_t reserve = 64) { stack.reserve(reserve); }

  Value* local_slot(size_t frame_index, uint8_t slot) {
    auto bp = frames[frame_index].bp;
    return &stack[bp + slot];
  }
  std::shared_ptr<Upvalue> capture_upvalue(Value* slot) {
    for (auto& uv : open_upvalues)
      if (uv->location == slot)
        return uv;
    auto uv = std::make_shared<Upvalue>();
    uv->location = slot;
    open_upvalues.push_back(uv);
    return uv;
  }
  void close_upvalues_from(Value* from) {
    for (auto& uv : open_upvalues) {
      if (uv->location >= from) {
        uv->closed = *uv->location;
        uv->location = &uv->closed;
        uv->is_closed = true;
      }
    }
    open_upvalues.erase(
        std::remove_if(open_upvalues.begin(), open_upvalues.end(),
                       [&](auto& u) { return u->is_closed; }),
        open_upvalues.end());
  }

  ObjType Type() const noexcept override { return ObjType::Fiber; }
  std::string Str() const override { return "fiber"; }
  std::string Desc() const override { return "<fiber>"; }
};

}  // namespace serilang
