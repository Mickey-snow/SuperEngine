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

#include "srbind/args.hpp"
#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <array>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace srbind {
namespace detail {

inline size_t arg_base(serilang::Fiber& f, size_t nargs, size_t nkwargs) {
  // Same layout as the old NativeFunction adaptor.
  return f.stack.size() - nkwargs * 2 - nargs;
}

inline Value& arg_at(serilang::Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     size_t i) {
  size_t base = arg_base(f, nargs, nkwargs);
  return f.stack[base + i];
}

// Pop all args and shrink the stack; leave callee slot in place.
inline void drop_args(serilang::Fiber& f, size_t nargs, size_t nkwargs) {
  f.stack.resize(arg_base(f, nargs, nkwargs));
}

// read kwargs into a map<name, Value*>
inline auto read_kwargs(serilang::Fiber& f, size_t nargs, size_t nkwargs)
    -> std::unordered_map<std::string, Value*> {
  std::unordered_map<std::string, Value*> m;
  m.reserve(nkwargs);
  size_t base = arg_base(f, nargs, nkwargs);
  size_t idx = base + nargs;
  for (size_t i = 0; i < nkwargs; ++i) {
    std::string* k = f.stack[idx++].Get_if<std::string>();
    if (!k)
      throw type_error("keyword name must be string");
    Value& v = f.stack[idx++];
    m.emplace(*k, &v);
  }
  return m;
}

// load tuple<Args...> using optional spec (names/defaults) and an offset:
// offset=0 for free functions, offset=1 for methods (skip 'self').
template <class... Args, size_t... I>
std::tuple<Args...> load_mapped_impl(serilang::VM& vm,
                                     serilang::Fiber& f,
                                     size_t nargs,
                                     size_t nkwargs,
                                     const std::vector<arg_t>* spec,
                                     size_t offset,
                                     std::index_sequence<I...>) {
  constexpr size_t N = sizeof...(Args);
  if (spec && !spec->empty() && spec->size() != N)
    throw type_error("binder: number of names/defaults doesn't match arity");

  size_t base = arg_base(f, nargs, nkwargs);
  if (nargs < offset)
    throw type_error("binder: missing 'self'");

  // kwargs map
  auto kw = read_kwargs(f, nargs, nkwargs);
  // track used kw to report unexpected leftovers
  std::unordered_set<std::string> used;

  // storage for defaults so Value& stays valid during cast
  std::vector<Value> default_vals;
  default_vals.reserve(N);

  // resolve a Value* source for param i
  [[maybe_unused]] auto pick_src = [&](size_t i) -> Value* {
    Value* src = nullptr;
    const bool have_pos = (offset + i) < nargs;
    const bool have_spec = spec && i < spec->size();
    const std::string name = (have_spec ? (*spec)[i].name : "");

    if (have_pos) {
      src = &f.stack[base + offset + i];
      // duplicated by kw?
      if (!name.empty()) {
        auto it = kw.find(name);
        if (it != kw.end())
          throw type_error("multiple values for argument '" + name + "'");
      }
      return src;
    }

    // try keyword by name
    if (!name.empty()) {
      auto it = kw.find(name);
      if (it != kw.end()) {
        used.insert(name);
        return it->second;
      }
    }

    // default
    if (have_spec && (*spec)[i].has_default) {
      serilang::TempValue tv = (*spec)[i].make_default();
      default_vals.push_back(vm.gc_->TrackValue(std::move(tv)));
      return &default_vals.back();
    }

    // missing
    std::string label =
        name.empty() ? ("#" + std::to_string(i)) : ("'" + name + "'");
    throw type_error("missing required argument " + label);
  };

  // collect sources
  [[maybe_unused]] std::array<Value*, N> srcs{(pick_src(I))...};

  // any unexpected keywords left?
  if (spec && !spec->empty()) {
    for (auto& [k, v] : kw) {
      if (!used.count(k)) {
        // not matched to any named parameter
        throw type_error("unexpected keyword argument '" + k + "'");
      }
    }
  } else {
    if (!kw.empty())
      throw type_error("function takes no keyword arguments");
  }

  // convert to tuple<Args...>
  return std::tuple<Args...>{type_caster<Args>::load(*srcs[I])...};
}

template <class... Args>
std::tuple<Args...> load_mapped(serilang::VM& vm,
                                serilang::Fiber& f,
                                size_t nargs,
                                size_t nkwargs,
                                const std::vector<arg_t>* spec,
                                size_t offset) {
  // too many positional?
  constexpr size_t N = sizeof...(Args);
  if (nargs > offset + N)
    throw type_error("expected at most " + std::to_string(N) +
                     " positional arguments");
  return load_mapped_impl<Args...>(vm, f, nargs, nkwargs, spec, offset,
                                   std::index_sequence_for<Args...>{});
}

template <class T>
T from_value(Value& v) {
  return type_caster<T>::load(v);
}

template <class R>
serilang::TempValue to_value(R&& r) {
  return type_caster<std::decay_t<R>>::cast(std::forward<R>(r));
}
inline serilang::Value to_value(void) { return serilang::nil; }

template <class... Args, size_t... I>
std::tuple<Args...> load_positional(serilang::Fiber& f,
                                    size_t nargs,
                                    size_t nkwargs,
                                    std::index_sequence<I...>) {
  if (sizeof...(Args) != nargs)
    throw type_error("expected " + std::to_string(sizeof...(Args)) +
                     " positional args, got " + std::to_string(nargs));
  return std::tuple<Args...>{from_value<Args>(arg_at(f, nargs, nkwargs, I))...};
}

template <class F, class Tuple, size_t... I>
auto apply(F&& f, Tuple&& t, std::index_sequence<I...>) {
  return std::invoke(std::forward<F>(f),
                     std::get<I>(std::forward<Tuple>(t))...);
}

}  // namespace detail
}  // namespace srbind
