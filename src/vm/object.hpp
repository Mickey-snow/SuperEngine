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

#include "utilities/transparent_hash.hpp"
#include "vm/call_frame.hpp"
#include "vm/instruction.hpp"
#include "vm/iobject.hpp"
#include "vm/value.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace serilang {

struct Code final : public IObject {
  static constexpr inline ObjType objtype = ObjType::Code;
  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }
  std::string Str() const override;
  std::string Desc() const override;

  std::vector<std::byte> code;
  std::vector<Value> const_pool;

  void MarkRoots(GCVisitor& visitor) final;

  [[nodiscard]] inline std::byte operator[](const std::size_t idx) const {
    return code[idx];
  }

  // helpers ------------------------------------------------------
  // Encode *one* concrete instruction type and push its bytes.
  template <typename T>
    requires(std::constructible_from<Instruction, T> &&
             std::is_trivially_copyable_v<T>)
  void Append(T v) {
    // opcode byte
    code.emplace_back(std::bit_cast<std::byte>(GetOpcode<T>()));

    // payload
    if constexpr (sizeof(T) > 0) {
      auto* p = reinterpret_cast<const std::byte*>(&v);
      code.insert(code.end(), p, p + sizeof(T));
    }
  }

  // Variant-aware overload – forwards to the typed version above.
  inline void Append(Instruction ins) {
    std::visit([this](auto&& op) { Append(std::move(op)); }, std::move(ins));
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void Write(const std::size_t idx, T v) {
    auto* p = reinterpret_cast<std::byte*>(&v);
    std::copy(p, p + sizeof(T), code.begin() + idx);
  }

  template <typename T>
  [[nodiscard]] inline T Read(std::size_t ip) const {
    using ptr_t = std::add_pointer_t<std::add_const_t<T>>;
    auto* p = reinterpret_cast<ptr_t>(code.data() + ip);
    return *p;
  }
};

struct Class : public IObject {
  static constexpr inline ObjType objtype = ObjType::Class;

  std::string name;
  transparent_hashmap<Function*> memfns;
  transparent_hashmap<Value> fields;

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
  transparent_hashmap<Value> fields;
  explicit Instance(Class* klass_);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  TempValue Member(std::string_view mem) override;
  void SetMember(std::string_view mem, Value val) override;
  void GetItem(VM& vm, Fiber& f) override;
  void SetItem(VM& vm, Fiber& f) override;
};

struct NativeClass : public IObject {
  static constexpr inline ObjType objtype = ObjType::NativeClass;

  using finalize_fn = void (*)(void*);
  using trace_fn = void (*)(GCVisitor&, void*);

  std::string name;
  transparent_hashmap<Value> methods;
  finalize_fn finalize = nullptr;
  trace_fn trace = nullptr;

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) override;
};

struct NativeInstance : public IObject {
  static constexpr inline ObjType objtype = ObjType::NativeInstance;

  NativeClass* klass;
  transparent_hashmap<Value> fields;
  void* foreign = nullptr;

  explicit NativeInstance(NativeClass* klass_);
  ~NativeInstance() override;

  template <typename T>
  void SetForeign(T* ptr) {
    foreign = ptr;
  }
  template <typename T>
  T* GetForeign() const {
    return static_cast<T*>(foreign);
  }

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
  std::optional<Value> pending_result = std::nullopt;
  Fiber* waiter = nullptr;
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

  void GetItem(VM& vm, Fiber& f) override;
  void SetItem(VM& vm, Fiber& f) override;
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

  void GetItem(VM& vm, Fiber& f) override;
  void SetItem(VM& vm, Fiber& f) override;
};

struct Module : public IObject {
  static constexpr inline ObjType objtype = ObjType::Module;

  std::string name;
  Dict* globals;

  explicit Module(std::string in_name, Dict* in_globals);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  TempValue Member(std::string_view mem) override;
  void SetMember(std::string_view mem, Value value) override;
};

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

class NativeFunction : public IObject {
 public:
  static constexpr inline ObjType objtype = ObjType::Native;

  using function_t = std::function<TempValue(VM&,
                                             Fiber&,
                                             uint8_t,
                                             uint8_t  // nargs, nkwargs
                                             )>;

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

struct BoundMethod : public IObject {
  static constexpr inline ObjType objtype = ObjType::BoundMethod;

  std::vector<Value> additional_args;
  Value method;

  BoundMethod(Value receiver, Value fn);
  BoundMethod(Value fn, std::vector<Value> add_args);

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;

  void Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) override;
};

}  // namespace serilang
