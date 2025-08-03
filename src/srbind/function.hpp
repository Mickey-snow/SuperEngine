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

#include "srbind/arglist_spec.hpp"
#include "srbind/argloader.hpp"
#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <type_traits>
#include <utility>
#include <vector>

namespace srbind {

namespace detail {

// SFINAE-friendly invoke for free functions
template <class F, class R, class... Args>
auto invoke_free_impl(serilang::VM& vm,
                      serilang::Fiber& f,
                      size_t nargs,
                      size_t nkwargs,
                      F&& fn,
                      R (*)(Args...),
                      const arglist_spec& spec) -> Value {
  auto tup = load_args<Args...>(f.stack, nargs, nkwargs, spec);
  if constexpr (std::is_void_v<R>) {
    std::apply(std::forward<F>(fn), tup);
    return serilang::nil;
  } else {
    R r = std::apply(std::forward<F>(fn), tup);
    serilang::TempValue tv = type_caster<std::decay_t<R>>::cast(std::move(r));
    return vm.gc_->TrackValue(std::move(tv));
  }
}

template <class F, class C, class R, class... Args>
auto invoke_free_sig(serilang::VM& vm,
                     serilang::Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     F&& fn,
                     R (C::*)(Args...) const,
                     const arglist_spec& spec) -> Value {
  return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                          (R (*)(Args...)) nullptr, spec);
}

template <class F, class C, class R, class... Args>
auto invoke_free_sig(serilang::VM& vm,
                     serilang::Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     F&& fn,
                     R (C::*)(Args...),
                     const arglist_spec& spec) -> Value {
  return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                          (R (*)(Args...)) nullptr, spec);
}

// helper to deduce F’s type
template <class F>
auto invoke_free(serilang::VM& vm,
                 serilang::Fiber& f,
                 size_t nargs,
                 size_t nkwargs,
                 F&& fn,
                 const arglist_spec& spec) -> Value {
  using Fn = std::decay_t<F>;
  if constexpr (std::is_function_v<Fn>) {
    using R = std::invoke_result_t<Fn>;
    return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                            (R (*)(void)) nullptr, spec);
  } else if constexpr (std::is_pointer_v<Fn> &&
                       std::is_function_v<std::remove_pointer_t<Fn>>) {
    using R = std::invoke_result_t<Fn>;
    return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                            (R (*)(void)) nullptr, spec);
  } else {
    using Sig = decltype(&Fn::operator());
    return invoke_free_sig(vm, f, nargs, nkwargs, std::forward<F>(fn), Sig{},
                           spec);
  }
}

template <class F>
serilang::NativeFunction* make_function(serilang::GarbageCollector* gc,
                                        std::string name,
                                        F&& f,
                                        arglist_spec spec) {
  return gc->Allocate<serilang::NativeFunction>(
      std::move(name),
      [fn = std::forward<F>(f), spec = std::move(spec)](
          serilang::VM& vm, serilang::Fiber& fib, uint8_t nargs,
          uint8_t nkwargs) -> Value {
        try {
          return invoke_free(vm, fib, nargs, nkwargs, fn, spec);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

}  // namespace detail

// -------------------------------------------------------------
// wrap free function: def("name", callable)
// -------------------------------------------------------------
template <class F, class... A>
serilang::NativeFunction* make_function(serilang::GarbageCollector* gc,
                                        std::string name,
                                        F&& f,
                                        A&&... a) {
  return detail::make_function(gc, std::move(name), std::forward<F>(f),
                               parse_spec(std::forward<A>(a)...));
}

template <class F>
serilang::NativeFunction* make_function(serilang::GarbageCollector* gc,
                                        std::string name,
                                        F&& f) {
  return detail::make_function(gc, std::move(name), std::forward<F>(f),
                               parse_spec<F>());
}

}  // namespace srbind
