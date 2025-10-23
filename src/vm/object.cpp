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

#include "vm/object.hpp"

#include "utilities/string_utilities.hpp"
#include "vm/function.hpp"
#include "vm/promise.hpp"
#include "vm/string.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <format>
#include <optional>

namespace serilang {

std::string Code::Str() const { return "<code>"; }
std::string Code::Desc() const { return "<code>"; }

void Code::MarkRoots(GCVisitor& visitor) {
  for (auto it : const_pool)
    visitor.MarkSub(it);
}

// -----------------------------------------------------------------------
void Class::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, it] : memfns)
    visitor.MarkSub(it);
  for (auto& [k, it] : fields)
    visitor.MarkSub(it);
}

std::string Class::Str() const { return Desc(); }

std::string Class::Desc() const { return "<class " + name + '>'; }

void Class::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  Instance* inst = vm.gc_->Allocate<Instance>(this);
  auto init_it = memfns.find("__init__");
  if (init_it != memfns.cend()) {
    Function* init_fn = init_it->second;

    // (class, args..., kwargs...)
    auto base = f.op_stack.end() - nargs - 2 * nkwargs - 1;
    *base = Value(inst);
    f.op_stack.insert(base, Value(init_fn));
    // (init, inst, args..., kwargs...)
    // here, we do a manual tail-call
    init_fn->Call(vm, f, nargs + 1, nkwargs);
    // ctors are guaranteed to return the instance
  } else {
    if (nargs != 0 || nkwargs != 0)
      throw RuntimeError(Str() + " takes no arguments");
    f.op_stack.back() = Value(inst);
  }
}

// -----------------------------------------------------------------------
Instance::Instance(Class* klass_) : klass(klass_) {}

void Instance::MarkRoots(GCVisitor& visitor) {
  klass->MarkRoots(visitor);
  for (auto& [k, it] : fields)
    visitor.MarkSub(it);
}

std::string Instance::Str() const { return Desc(); }

std::string Instance::Desc() const { return '<' + klass->name + " object>"; }

TempValue Instance::Member(std::string_view mem) {
  {
    auto it = fields.find(mem);
    if (it != fields.cend())
      return it->second;
  }
  {
    auto it = klass->memfns.find(mem);
    if (it != klass->memfns.cend())
      return std::make_unique<BoundMethod>(Value(this), Value(it->second));
  }
  {
    auto it = klass->fields.find(mem);
    if (it != klass->fields.cend())
      return it->second;
  }

  throw RuntimeError(
      std::format("'{}' object has no member '{}'", Desc(), mem));
}

void Instance::SetMember(std::string_view mem, Value val) {
  fields[std::string(mem)] = std::move(val);
}

void Instance::GetItem(VM& vm, Fiber& f) {
  // (..., inst, idx)

  Value& idx = f.op_stack.end()[-1];
  auto it = klass->memfns.find("__getitem__");
  if (it == klass->memfns.end()) {
    vm.Error(f, std::format("'{}' object has no item '{}'", Desc(), idx.Str()));
    return;
  }

  f.op_stack.insert(f.op_stack.end() - 2, Value(it->second));
  it->second->Call(vm, f, 2, 0);
  // (..., __getitem__, inst, idx) -> (..., result)
}

void Instance::SetItem(VM& vm, Fiber& f) {
  // (..., inst, idx, val)
  auto it = klass->memfns.find("__setitem__");
  if (it == klass->memfns.end()) {
    vm.Error(
        f, std::format("'{}' object does not support item assignment", Desc()));
    return;
  }

  f.op_stack.insert(f.op_stack.end() - 3, Value(it->second));
  it->second->Call(vm, f, 3, 0);
  // (..., __setitem__, inst, idx, val) -> (...)
}

// -----------------------------------------------------------------------
void NativeClass::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, it] : methods)
    visitor.MarkSub(it);
}

std::string NativeClass::Str() const { return Desc(); }

std::string NativeClass::Desc() const { return "<native class " + name + '>'; }

void NativeClass::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  NativeInstance* inst = vm.gc_->Allocate<NativeInstance>(this);

  auto init_it = methods.find("__init__");
  if (init_it != methods.cend()) {
    Value init_fn = init_it->second;

    auto base = f.op_stack.end() - nargs - 2 * nkwargs - 1;
    *base = Value(inst);
    f.op_stack.insert(base, init_fn);
    // (__init__, inst, args, ...)
    init_fn.Call(vm, f, nargs + 1, nkwargs);
    // -> (nil)
  } else {
    if (nargs != 0 || nkwargs != 0)
      throw RuntimeError(Str() + " takes no arguments");
  }

  f.op_stack.back() = Value(inst);  // -> (inst)
}

// -----------------------------------------------------------------------
NativeInstance::NativeInstance(NativeClass* klass_) : klass(klass_) {}

NativeInstance::~NativeInstance() {
  if (klass->finalize && foreign)
    klass->finalize(foreign);
}

void NativeInstance::MarkRoots(GCVisitor& visitor) {
  klass->MarkRoots(visitor);
  for (auto& [k, it] : fields)
    visitor.MarkSub(it);
  if (klass->trace && foreign)
    klass->trace(visitor, foreign);
}

std::string NativeInstance::Str() const { return Desc(); }

std::string NativeInstance::Desc() const {
  return "<" + klass->name + " object>";
}

TempValue NativeInstance::Member(std::string_view mem) {
  {
    auto it = fields.find(mem);
    if (it != fields.cend())
      return it->second;
  }

  {
    auto it = klass->methods.find(mem);
    if (it != klass->methods.cend()) {
      Value val = it->second;
      if (IObject* obj = val.Get_if<IObject>()) {
        auto t = obj->Type();
        if (t == ObjType::Function || t == ObjType::Native)
          return std::make_unique<BoundMethod>(Value(this), val);
      }
      return val;
    }
  }

  throw RuntimeError('\'' + Desc() + "' object has no member '" +
                     std::string(mem) + '\'');
}

void NativeInstance::SetMember(std::string_view mem, Value val) {
  fields[std::string(mem)] = std::move(val);
}

void NativeInstance::GetItem(VM& vm, Fiber& f) {
  Value& idx = f.op_stack.back();

  auto lookup = [this](std::string_view name) -> std::optional<Value> {
    if (auto it = fields.find(name); it != fields.end())
      return it->second;
    if (auto it = klass->methods.find(name); it != klass->methods.end())
      return it->second;
    return std::nullopt;
  };

  auto callee = lookup("__getitem__");
  if (!callee) {
    vm.Error(f, std::format("'{}' object has no item '{}'", Desc(), idx.Str()));
    return;
  }

  f.op_stack.insert(f.op_stack.end() - 2, *callee);
  f.op_stack.end()[-3].Call(vm, f, 2, 0);
}

void NativeInstance::SetItem(VM& vm, Fiber& f) {
  auto lookup = [this](std::string_view name) -> std::optional<Value> {
    if (auto it = fields.find(name); it != fields.end())
      return it->second;
    if (auto it = klass->methods.find(name); it != klass->methods.end())
      return it->second;
    return std::nullopt;
  };

  auto callee = lookup("__setitem__");
  if (!callee) {
    vm.Error(
        f, std::format("'{}' object does not support item assignment", Desc()));
    return;
  }

  f.op_stack.insert(f.op_stack.end() - 3, *callee);
  f.op_stack.end()[-4].Call(vm, f, 3, 0);
}

// -----------------------------------------------------------------------
Fiber::Fiber(size_t reserve) : state(FiberState::New) {
  op_stack.reserve(reserve);
  ResetPromise();
}

void Fiber::MarkRoots(GCVisitor& visitor) {
  // mark completion promise's payload
  for (IObject* it : completion_promise->roots)
    visitor.MarkSub(it);

  for (auto& it : op_stack)
    visitor.MarkSub(it);
  for (auto& it : frames)
    visitor.MarkSub(it.fn);
  for (auto& it : open_upvalues)
    if (it->location)
      visitor.MarkSub(*it->location);
}

std::shared_ptr<Upvalue> Fiber::capture_upvalue(Value* slot) {
  for (auto& uv : open_upvalues) {
    if (uv->location == slot) {
      return uv;
    }
  }
  auto uv = std::make_shared<Upvalue>();
  uv->location = slot;
  open_upvalues.push_back(uv);
  return uv;
}

void Fiber::close_upvalues_from(Value* from) {
  for (auto& uv : open_upvalues) {
    if (uv->location >= from) {
      uv->closed = *uv->location;
      uv->location = &uv->closed;
      uv->is_closed = true;
    }
  }
  open_upvalues.erase(std::remove_if(open_upvalues.begin(), open_upvalues.end(),
                                     [&](auto& u) { return u->is_closed; }),
                      open_upvalues.end());
}

std::string Fiber::Str() const { return "fiber"; }

std::string Fiber::Desc() const { return "<fiber>"; }

std::optional<Value>& Fiber::GetFastLocal(size_t slot) {
  CallFrame& frame = frames.back();
  return frame.fast_locals.at(slot);
}

std::string_view Fiber::GetFastLocalName(size_t slot) {
  CallFrame& frame = frames.back();
  return frame.fn->chunk->fast_locals.at(slot);
}

void Fiber::Yield(Value result) {
  if (state == FiberState::Dead)
    throw std::logic_error("Fiber: Yield from a dead fiber");

  completion_promise->Resolve(std::move(result));
  ResetPromise();

  state = FiberState::Suspended;
}

void Fiber::Kill(expected<Value, std::string> result) {
  if (state == FiberState::Dead)
    throw std::logic_error("Fiber: Killing a dead fiber");

  state = FiberState::Dead;
  completion_promise->Provide(std::move(result));
  // let the completion promise hold final result, don't reset promise here

  frames.clear();
  op_stack.clear();
}

// -----------------------------------------------------------------------
std::string List::Str() const {
  return '[' +
         Join(",", std::views::all(items) |
                       std::views::transform(
                           [](const Value& v) { return v.Str(); })) +
         ']';
}
std::string List::Desc() const {
  return "<list[" + std::to_string(items.size()) + "]>";
}

void List::MarkRoots(GCVisitor& visitor) {
  for (auto& it : items)
    visitor.MarkSub(it);
}

void List::GetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.back());
  f.op_stack.pop_back();

  auto* index_ptr = idx.Get_if<int>();
  if (!index_ptr) {
    vm.Error(f, "list index must be integer, but got: " + idx.Desc());
    return;
  }

  int index = *index_ptr < 0 ? items.size() + *index_ptr : *index_ptr;
  if (index < 0 || index >= items.size()) {
    vm.Error(f, "list index '" + idx.Str() + "' out of range");
    return;
  }

  f.op_stack.back() = items[index];
}

void List::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.end()[-2]),
        val = std::move(f.op_stack.end()[-1]);
  f.op_stack.resize(f.op_stack.size() - 3);  // (list, idx, val)

  auto* index_ptr = idx.Get_if<int>();
  if (!index_ptr) {
    vm.Error(f, "list index must be integer, but got: " + idx.Desc());
    return;
  }

  int index = *index_ptr < 0 ? items.size() + *index_ptr : *index_ptr;
  if (index < 0 || index >= items.size()) {
    vm.Error(f, "list index '" + idx.Str() + "' out of range");
    return;
  }

  items[index] = std::move(val);
}

// -----------------------------------------------------------------------
std::string Dict::Str() const {
  std::string repr;

  for (const auto& [k, v] : map) {
    if (!repr.empty())
      repr += ',';
    repr += k + ':' + v.Str();
  }

  return '{' + repr + '}';
}
std::string Dict::Desc() const {
  return "<dict{" + std::to_string(map.size()) + "}>";
}

void Dict::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, it] : map)
    visitor.MarkSub(it);
}

void Dict::GetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.back());
  f.op_stack.pop_back();
  String const* index = idx.Get_if<const String>();
  if (!index) {
    vm.Error(f, "dictionary index must be string, but got: " + idx.Desc());
    return;
  }

  auto it = map.find(index->str_);
  if (it == map.cend()) {
    vm.Error(f, "dictionary has no key: " + idx.Str());
    return;
  }

  f.op_stack.back() = it->second;
}

void Dict::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.end()[-2]),
        val = std::move(f.op_stack.end()[-1]);
  f.op_stack.resize(f.op_stack.size() - 3);  // (dict, idx, val)
  String const* index = idx.Get_if<const String>();
  if (!index) {
    vm.Error(f, "dictionary index must be string, but got: " + idx.Desc());
    return;
  }

  map[index->str_] = std::move(val);
}

// -----------------------------------------------------------------------
Module::Module(std::string in_name, Dict* in_globals)
    : name(std::move(in_name)), globals(in_globals) {}

void Module::MarkRoots(GCVisitor& visitor) { visitor.MarkSub(globals); }

std::string Module::Str() const { return "<module " + name + ">"; }

std::string Module::Desc() const { return "<module " + name + ">"; }

TempValue Module::Member(std::string_view mem) {
  std::string mem_str(mem);
  auto it = globals->map.find(mem_str);
  if (it == globals->map.cend())
    throw RuntimeError("module '" + name + "' has no attribute '" + mem_str +
                       '\'');
  return it->second;
}

void Module::SetMember(std::string_view mem, Value value) {
  globals->map[std::string(mem)] = std::move(value);
}

// -----------------------------------------------------------------------
NativeFunction::NativeFunction(std::string name, function_t fn)
    : name_(std::move(name)), fn_(std::move(fn)) {}

std::string NativeFunction::Name() const { return name_; }

std::string NativeFunction::Str() const { return "<fn " + name_ + '>'; }

std::string NativeFunction::Desc() const {
  return "<native function '" + name_ + "'>";
}

void NativeFunction::MarkRoots(GCVisitor& visitor) {}

void NativeFunction::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  TempValue retval = std::invoke(fn_, vm, f, nargs, nkwargs);

  f.op_stack.back() = vm.AddTrack(std::move(retval));  // (fn) <- (retval)
}

// -----------------------------------------------------------------------
BoundMethod::BoundMethod(Value recv, Value fn)
    : BoundMethod(std::move(fn), std::vector<Value>{std::move(recv)}) {}

BoundMethod::BoundMethod(Value fn, std::vector<Value> add_args)
    : additional_args(std::move(add_args)), method(std::move(fn)) {}

void BoundMethod::MarkRoots(GCVisitor& visitor) {
  for (Value& it : additional_args)
    visitor.MarkSub(it);
  visitor.MarkSub(method);
}

std::string BoundMethod::Str() const { return Desc(); }

std::string BoundMethod::Desc() const { return "<bound method>"; }

void BoundMethod::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  size_t base = f.op_stack.size() - nargs - 2 * nkwargs - 1;
  f.op_stack[base] = method;
  f.op_stack.insert(f.op_stack.begin() + base + 1, additional_args.cbegin(),
                    additional_args.cend());
  method.Call(vm, f, nargs + additional_args.size(), nkwargs);
}

}  // namespace serilang
