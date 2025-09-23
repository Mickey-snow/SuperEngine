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
#include "vm/promise.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <format>

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
    auto base = f.stack.end() - nargs - 2 * nkwargs - 1;
    *base = Value(inst);
    f.stack.insert(base, Value(init_fn));
    // (init, inst, args..., kwargs...)
    // here, we do a manual tail-call
    init_fn->Call(vm, f, nargs + 1, nkwargs);
    // ctors are guaranteed to return the instance
  } else {
    if (nargs != 0 || nkwargs != 0)
      throw RuntimeError(Str() + " takes no arguments");
    f.stack.back() = Value(inst);
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

  Value& idx = f.stack.end()[-1];
  auto it = klass->memfns.find("__getitem__");
  if (it == klass->memfns.end()) {
    vm.Error(f, std::format("'{}' object has no item '{}'", Desc(), idx.Str()));
    return;
  }

  f.stack.insert(f.stack.end() - 2, Value(it->second));
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

  f.stack.insert(f.stack.end() - 3, Value(it->second));
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

    auto base = f.stack.end() - nargs - 2 * nkwargs - 1;
    *base = Value(inst);
    f.stack.insert(base, init_fn);
    // (__init__, inst, args, ...)
    init_fn.Call(vm, f, nargs + 1, nkwargs);
    // -> (nil)
  } else {
    if (nargs != 0 || nkwargs != 0)
      throw RuntimeError(Str() + " takes no arguments");
  }

  f.stack.back() = Value(inst);  // -> (inst)
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

// -----------------------------------------------------------------------
Fiber::Fiber(size_t reserve) : state(FiberState::New) {
  stack.reserve(reserve);
  ResetPromise();
}

void Fiber::MarkRoots(GCVisitor& visitor) {
  if (pending_result.has_value())
    visitor.MarkSub(*pending_result);
  // mark completion promise's payload
  for (IObject* it : completion_promise->roots)
    visitor.MarkSub(it);

  for (auto& it : stack)
    visitor.MarkSub(it);
  for (auto& it : frames)
    visitor.MarkSub(it.fn);
  for (auto& it : open_upvalues)
    if (it->location)
      visitor.MarkSub(*it->location);
}

Value* Fiber::local_slot(size_t frame_index, uint8_t slot) {
  auto bp = frames[frame_index].bp;
  return &stack[bp + slot];
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

void Fiber::ResetPromise() { completion_promise = std::make_shared<Promise>(); }

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
  Value idx = std::move(f.stack.back());
  f.stack.pop_back();

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

  f.stack.back() = items[index];
}

void List::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.stack.end()[-2]), val = std::move(f.stack.end()[-1]);
  f.stack.resize(f.stack.size() - 3);  // (list, idx, val)

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
  Value idx = std::move(f.stack.back());
  f.stack.pop_back();
  auto* index = idx.Get_if<std::string>();
  if (!index) {
    vm.Error(f, "dictionary index must be string, but got: " + idx.Desc());
    return;
  }

  auto it = map.find(*index);
  if (it == map.cend()) {
    vm.Error(f, "dictionary has no key: " + idx.Str());
    return;
  }

  f.stack.back() = it->second;
}

void Dict::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.stack.end()[-2]), val = std::move(f.stack.end()[-1]);
  f.stack.resize(f.stack.size() - 3);  // (dict, idx, val)
  auto* index = idx.Get_if<std::string>();
  if (!index) {
    vm.Error(f, "dictionary index must be string, but got: " + idx.Desc());
    return;
  }

  map[*index] = std::move(val);
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
Function::Function(Code* in_chunk, uint32_t in_entry, uint32_t in_nparam)
    : chunk(in_chunk),
      entry(in_entry),
      nparam(in_nparam),
      param_index(),
      defaults(),
      has_vararg(false),
      has_kwarg(false) {}

std::string Function::Str() const { return "function"; }

std::string Function::Desc() const { return "<function>"; }

void Function::MarkRoots(GCVisitor& visitor) {
  visitor.MarkSub(globals);
  visitor.MarkSub(chunk);
  for (auto& [k, v] : defaults)
    visitor.MarkSub(v);
}

void Function::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::vector<Value>& stack = f.stack;

  if (nargs > nparam && !has_vararg) {
    vm.Error(f, Desc() + ": too many arguments");
    return;
  }

  // Set up call stack:
  // (fn, pos_arg1, (nargs)..., kw1, kw_arg1, (nkwargs)...)
  const auto base = stack.size() - nargs - 2 * nkwargs - 1;

  // pull arguments & kwargs off the stack
  std::vector<Value> posargs(
      std::make_move_iterator(stack.begin() + base + 1),
      std::make_move_iterator(stack.begin() + base + 1 + nargs));
  posargs.reserve(std::max<size_t>(nparam, nargs));
  std::unordered_map<std::string, Value> kwargs;
  kwargs.reserve(nkwargs);
  size_t idx = base + 1 + nargs;
  for (uint8_t i = 0; i < nkwargs; ++i) {
    std::string* k = stack[idx++].Get_if<std::string>();
    Value v = std::move(stack[idx++]);
    kwargs.emplace(std::move(*k), std::move(v));
  }
  stack.resize(base + 1);  // leave the callee on stack

  std::vector<Value> finalargs(nparam);
  std::vector<bool> assigned(nparam, false);

  for (size_t i = 0; i < posargs.size() && i < nparam; ++i) {
    finalargs[i] = std::move(posargs[i]);
    assigned[i] = true;
  }

  std::vector<Value> rest;
  if (posargs.size() > nparam)
    rest.assign(std::make_move_iterator(posargs.begin() + nparam),
                std::make_move_iterator(posargs.end()));

  std::unordered_map<std::string, Value> extra_kwargs;
  for (auto& [k, v] : kwargs) {
    auto it = param_index.find(k);
    if (it != param_index.end()) {
      auto idx = it->second;
      if (assigned[idx]) {
        vm.Error(f, Desc() + ": multiple values for argument '" + k + "'");
        return;
      }
      finalargs[idx] = std::move(v);
      assigned[idx] = true;
    } else {
      extra_kwargs.emplace(k, std::move(v));
    }
  }

  for (size_t i = 0; i < nparam; ++i) {
    if (!assigned[i]) {
      const auto it = defaults.find(i);
      if (it == defaults.cend()) {
        vm.Error(f, Desc() + ": missing arguments");
        return;
      }
      finalargs[i] = it->second;
    }
  }

  std::move(finalargs.begin(), finalargs.end(), std::back_inserter(stack));

  if (has_vararg) {
    stack.emplace_back(vm.gc_->Allocate<List>(std::move(rest)));
  }

  if (has_kwarg) {
    stack.emplace_back(vm.gc_->Allocate<Dict>(std::move(extra_kwargs)));
  } else if (!extra_kwargs.empty()) {
    vm.Error(f, Desc() + ": unexpected keyword argument");
    return;
  }

  // (fn, pos_arg1, ..., [var_arg], [kw_arg])
  f.frames.push_back({});
  auto& frame = f.frames.back();
  frame.fn = this;
  frame.ip = entry;
  frame.bp = base;
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

  f.stack.back() = vm.AddTrack(std::move(retval));  // (fn) <- (retval)
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
  size_t base = f.stack.size() - nargs - 2 * nkwargs - 1;
  f.stack[base] = method;
  f.stack.insert(f.stack.begin() + base + 1, additional_args.cbegin(),
                 additional_args.cend());
  method.Call(vm, f, nargs + additional_args.size(), nkwargs);
}

}  // namespace serilang
