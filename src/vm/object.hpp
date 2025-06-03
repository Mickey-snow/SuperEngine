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
#include "vm/chunk.hpp"
#include "vm/iobject.hpp"
#include "vm/value.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace serilang {

struct Class : public IObject {
  static constexpr inline ObjType objtype = ObjType::Class;

  std::string name;
  std::unordered_map<std::string, Value> methods;

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) override;
};

struct Instance : public IObject {
  static constexpr inline ObjType objtype = ObjType::Instance;

  Class* klass;
  std::unordered_map<std::string, Value> fields;
  explicit Instance(Class* klass_);
  
  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  TempValue Member(std::string_view mem) override;
  void SetMember(std::string_view mem, Value val) override;
};

enum class FiberState { New, Running, Suspended, Dead };

struct Upvalue;

struct Fiber : public IObject {
  static constexpr inline ObjType objtype = ObjType::Fiber;

  std::vector<Value> stack;
  std::vector<CallFrame> frames;
  FiberState state;
  Value last;
  std::vector<std::shared_ptr<Upvalue>> open_upvalues;

  explicit Fiber(size_t reserve = 64);

  Value* local_slot(size_t frame_index, uint8_t slot);
  std::shared_ptr<Upvalue> capture_upvalue(Value* slot);
  void close_upvalues_from(Value* from);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;
};

struct List : public IObject {
  static constexpr inline ObjType objtype = ObjType::List;

  std::vector<Value> items;

  explicit List(std::vector<Value> xs = {}) : items(std::move(xs)) {}

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;   // “[1, 2, 3]”
  std::string Desc() const override;  // “<list[3]>”
};

struct Dict : public IObject {
  static constexpr inline ObjType objtype = ObjType::Dict;

  std::unordered_map<std::string, Value> map;

  explicit Dict(std::unordered_map<std::string, Value> m = {})
      : map(std::move(m)) {}
  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;   // “{a: 1, b: 2}”
  std::string Desc() const override;  // “<dict{2}>”
};

class NativeFunction : public IObject {
 public:
  static constexpr inline ObjType objtype = ObjType::Native;

  using function_t =
      std::function<Value(Fiber&,
                          std::vector<Value>,
                          std::unordered_map<std::string, Value>)>;

  NativeFunction(std::string name, function_t fn);

  std::string Name() const;
  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) final;

 private:
  std::string name_;
  function_t fn_;
};

struct Closure : public IObject {
  static constexpr inline ObjType objtype = ObjType::Closure;

  std::shared_ptr<Chunk> chunk;
  uint32_t entry{};
  uint32_t nparams{};
  uint32_t nlocals{};

  explicit Closure(std::shared_ptr<Chunk> c);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) final;
};

}  // namespace serilang
