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

#include "vm/function.hpp"

#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <format>

namespace serilang {

expected<void, std::string> ArgumentList::Load(
    std::shared_ptr<GarbageCollector> gc,
    std::vector<Value>& stack,
    uint8_t nargs,
    uint8_t nkwargs) const {
  using std::string_literals::operator""s;

  if (nargs > nparam && !has_vararg)
    return unexpected("too many arguments"s);

  // Set up call stack:
  // (fn, pos_arg1, (nargs)..., kw1, kw_arg1, (nkwargs)...)
  const size_t base = stack.size() - nargs - 2 * nkwargs - 1;

  // pull arguments & kwargs off the stack
  std::vector<Value> posargs(
      std::make_move_iterator(stack.begin() + base + 1),
      std::make_move_iterator(stack.begin() + base + 1 + nargs));
  posargs.reserve(std::max<size_t>(nparam, nargs));
  std::unordered_map<std::string, Value> kwargs;
  kwargs.reserve(nkwargs);
  size_t idx = base + 1 + nargs;
  for (uint8_t i = 0; i < nkwargs; ++i) {
    std::string k = stack[idx++].Get_if<String>()->str_;
    Value v = std::move(stack[idx++]);
    kwargs.emplace(std::move(k), std::move(v));
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
        return unexpected(std::format("multiple values for argument '{}'", k));

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
        return unexpected("missing arguments"s);

      finalargs[i] = it->second;
    }
  }

  std::move(finalargs.begin(), finalargs.end(), std::back_inserter(stack));

  if (has_vararg) {
    stack.emplace_back(gc->Allocate<List>(std::move(rest)));
  }

  if (has_kwarg) {
    stack.emplace_back(gc->Allocate<Dict>(std::move(extra_kwargs)));
  } else if (!extra_kwargs.empty()) {
    return unexpected("unexpected keyword argument"s);
  }

  return {};
}

// -----------------------------------------------------------------------
Function::Function(Code* c) : chunk(c) {}

std::string Function::Str() const { return "function"; }

std::string Function::Desc() const { return "<function>"; }

void Function::MarkRoots(GCVisitor& visitor) {
  visitor.MarkSub(globals);
  visitor.MarkSub(chunk);
  for (auto& [k, v] : arglist.defaults)
    visitor.MarkSub(v);
}

void Function::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::vector<Value>& stack = f.stack;
  const size_t base = stack.size() - nargs - 2 * nkwargs - 1;

  // load arguments
  if (auto result = arglist.Load(vm.gc_, stack, nargs, nkwargs);
      !result.has_value()) {
    vm.Error(f, std::format("{}: {}", Desc(), std::move(result.error())));
    return;
  }
  // (fn, pos_arg1, ..., [var_arg], [kw_arg])

  f.frames.push_back({});
  auto& frame = f.frames.back();
  frame.fn = this;
  frame.ip = entry;
  frame.bp = base;
}

}  // namespace serilang
