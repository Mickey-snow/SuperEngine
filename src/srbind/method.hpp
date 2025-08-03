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

#include "srbind/function.hpp"

namespace srbind {

namespace detail {
template <class T, class R, class... Args>
Value do_invoke_method(serilang::VM& vm,
                       serilang::Fiber& f,
                       size_t nargs,
                       size_t nkwargs,
                       T* self,
                       R (T::*pmf)(Args...),
                       const arglist_spec& spec) {
  auto tup = load_args<Args...>(f.stack, nargs - 1, nkwargs, spec);
  f.stack.pop_back();  // self

  if constexpr (std::is_void_v<R>) {
    std::apply(
        [&](auto&&... a) { (self->*pmf)(std::forward<decltype(a)>(a)...); },
        std::move(tup));
    return serilang::nil;
  } else {
    R r = std::apply(
        [&](auto&&... a) {
          return (self->*pmf)(std::forward<decltype(a)>(a)...);
        },
        std::move(tup));
    serilang::TempValue tv = type_caster<std::decay_t<R>>::cast(std::move(r));
    return vm.gc_->TrackValue(std::move(tv));
  }
}

template <class T, class R, class... Args>
Value do_invoke_method(serilang::VM& vm,
                       serilang::Fiber& f,
                       size_t nargs,
                       size_t nkwargs,
                       T* self,
                       R (T::*pmf)(Args...) const,
                       const arglist_spec& spec) {
  return do_invoke_method(vm, f, nargs, nkwargs, self, (R (T::*)(Args...))pmf,
                          spec);
}

template <class T, class M>
Value invoke_method(serilang::VM& vm,
                    serilang::Fiber& f,
                    size_t nargs,
                    size_t nkwargs,
                    T* self,
                    M pmf,
                    const arglist_spec& spec) {
  return do_invoke_method(vm, f, nargs, nkwargs, self, pmf, spec);
}
}  // namespace detail

// -------------------------------------------------------------
// wrap member function: def("name", &T::method)
// BoundMethod inserts receiver as arg0; we convert it to T*
// -------------------------------------------------------------
template <class T, class M, class... A>
serilang::NativeFunction* make_method(serilang::GarbageCollector* gc,
                                      std::string name,
                                      M method,
                                      A&&... a) {
  return gc->Allocate<serilang::NativeFunction>(
      std::move(name),
      [method, spec = parse_spec(std::forward<A>(a)...)](
          serilang::VM& vm, serilang::Fiber& f, uint8_t nargs,
          uint8_t nkwargs) -> Value {
        try {
          if (nargs == 0)
            throw type_error("missing 'self'");
          Value& selfv = f.stack.end()[-nargs - 2 * nkwargs];
          T* self = type_caster<T*>::load(selfv);
          return detail::invoke_method(vm, f, nargs, nkwargs, self, method,
                                       spec);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

}  // namespace srbind
