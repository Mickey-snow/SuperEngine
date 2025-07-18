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
#include "vm/upvalue.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

namespace serilang {

std::string Code::Str() const { return "<code>"; }
std::string Code::Desc() const { return "<code>"; }

void Code::MarkRoots(GCVisitor& visitor) {
  for (auto it : const_pool)
    visitor.MarkSub(it);
}

// -----------------------------------------------------------------------
void Class::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, it] : methods)
    visitor.MarkSub(it);
}

std::string Class::Str() const { return Desc(); }

std::string Class::Desc() const { return "<class " + name + '>'; }

void Class::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  f.stack.resize(f.stack.size() - nargs);
  auto inst = vm.gc_.Allocate<Instance>(this);
  inst->fields = this->methods;
  f.stack.back() = Value(std::move(inst));
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
  auto it = fields.find(std::string(mem));
  if (it == fields.cend())
    throw std::runtime_error('\'' + Desc() + "' object has no member '" +
                             std::string(mem) + '\'');
  return it->second;
}
void Instance::SetMember(std::string_view mem, Value val) {
  fields[std::string(mem)] = val;
}

// -----------------------------------------------------------------------
Fiber::Fiber(size_t reserve) : state(FiberState::New) {
  stack.reserve(reserve);
}

void Fiber::MarkRoots(GCVisitor& visitor) {
  if (pending_result.has_value())
    visitor.MarkSub(*pending_result);
  visitor.MarkSub(waiter);

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

// -----------------------------------------------------------------------
void Module::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, v] : globals)
    visitor.MarkSub(v);
}

std::string Module::Str() const { return "<module " + name + ">"; }

std::string Module::Desc() const { return "<module " + name + ">"; }

TempValue Module::Member(std::string_view mem) {
  auto it = globals.find(std::string(mem));
  if (it == globals.end())
    throw std::runtime_error("module '" + name + "' has no member '" +
                             std::string(mem) + "'");
  return it->second;
}

void Module::SetMember(std::string_view mem, Value value) {
  globals[std::string(mem)] = value;
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
  visitor.MarkSub(chunk);
  for (auto& [k, v] : defaults)
    visitor.MarkSub(v);
}

void Function::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::vector<Value>& stack = f.stack;

  if (nargs > nparam && !has_vararg)
    vm.RuntimeError(Desc() + ": too many arguments");

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
      if (assigned[idx])
        vm.RuntimeError(Desc() + ": multiple values for argument '" + k + "'");
      finalargs[idx] = std::move(v);
      assigned[idx] = true;
    } else {
      extra_kwargs.emplace(k, std::move(v));
    }
  }

  for (size_t i = 0; i < nparam; ++i) {
    if (!assigned[i]) {
      const auto it = defaults.find(i);
      if (it == defaults.cend())
        vm.RuntimeError(Desc() + ": missing arguments");
      finalargs[i] = it->second;
    }
  }

  std::move(finalargs.begin(), finalargs.end(), std::back_inserter(stack));

  if (has_vararg) {
    stack.emplace_back(vm.gc_.Allocate<List>(std::move(rest)));
  }

  if (has_kwarg) {
    stack.emplace_back(vm.gc_.Allocate<Dict>(std::move(extra_kwargs)));
  } else if (!extra_kwargs.empty()) {
    vm.RuntimeError(Desc() + ": unexpected keyword argument");
  }

  // (fn, pos_arg1, ..., [var_arg], [kw_arg])
  f.frames.emplace_back(this, entry, base);
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
  std::vector<Value> args;
  std::unordered_map<std::string, Value> kwargs;

  args.reserve(nargs);
  kwargs.reserve(nkwargs);

  size_t idx = f.stack.size() - nkwargs * 2 - nargs;
  for (uint8_t i = 0; i < nargs; ++i)
    args.emplace_back(std::move(f.stack[idx++]));
  for (uint8_t i = 0; i < nkwargs; ++i) {
    std::string* k = f.stack[idx++].Get_if<std::string>();
    Value v = std::move(f.stack[idx++]);
    kwargs.emplace(std::move(*k), std::move(v));
  }

  auto retval = std::invoke(fn_, f, std::move(args), std::move(kwargs));

  f.stack.resize(f.stack.size() - nkwargs * 2 - nargs);
  f.stack.back() = std::move(retval);
}

}  // namespace serilang
