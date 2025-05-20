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
#include "vm/vm.hpp"

namespace serilang {

// -----------------------------------------------------------------------
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

std::string Instance::Str() const { return Desc(); }

std::string Instance::Desc() const { return '<' + klass->name + " object>"; }

// -----------------------------------------------------------------------
Fiber::Fiber(size_t reserve) : state(FiberState::New) {
  stack.reserve(reserve);
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

// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
Closure::Closure(std::shared_ptr<Chunk> c) : chunk(c) {}

std::string Closure::Str() const { return "closure"; }

std::string Closure::Desc() const { return "<closure>"; }

void Closure::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  if (nargs != this->nparams)
    throw std::runtime_error(Desc() + ": arity mismatch");

  const auto base = f.stack.size() - nparams - 1 /*fn slot*/;
  f.frames.push_back({this, entry, base});
}

// -----------------------------------------------------------------------
NativeFunction::NativeFunction(std::string name, function_t fn)
    : name_(std::move(name)), fn_(std::move(fn)) {}

std::string NativeFunction::Name() const { return name_; }

std::string NativeFunction::Str() const { return "<fn " + name_ + '>'; }

std::string NativeFunction::Desc() const {
  return "<native function '" + name_ + "'>";
}

void NativeFunction::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::vector<Value> args;
  std::unordered_map<std::string, Value> kwargs;

  args.reserve(nargs);
  for (size_t i = f.stack.size() - nargs; i < f.stack.size(); ++i)
    args.emplace_back(std::move(f.stack[i]));

  auto retval = std::invoke(fn_, f, std::move(args), std::move(kwargs));

  f.stack.resize(f.stack.size() - nargs);
  f.stack.back() = std::move(retval);
}

}  // namespace serilang
