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

// -------------------------------------------------------------
// wrap member function: def("name", &T::method)
// BoundMethod inserts receiver as arg0; we convert it to T*
// -------------------------------------------------------------
template <class T, class M, class... A>
serilang::NativeFunction* make_method(serilang::GarbageCollector* gc,
                                      std::string name,
                                      M method,
                                      A&&... a) {
  std::vector<arg_t> spec{std::forward<A>(a)...};
  return gc->Allocate<serilang::NativeFunction>(
      std::move(name),
      [method, spec = std::move(spec)](serilang::VM& vm, serilang::Fiber& f,
                                       uint8_t nargs,
                                       uint8_t nkwargs) -> Value {
        try {
          if (nargs == 0)
            throw type_error("missing 'self'");
          Value& selfv = detail::arg_at(f, nargs, nkwargs, 0);
          T* self = type_caster<T*>::load(selfv);
          return invoke_method(vm, f, nargs, nkwargs, self, method, &spec);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

template <class T, class R, class... Args>
Value do_invoke_method(serilang::VM& vm,
                       serilang::Fiber& f,
                       size_t nargs,
                       size_t nkwargs,
                       T* self,
                       R (T::*pmf)(Args...),
                       const std::vector<arg_t>* spec) {
  auto tup =
      detail::load_mapped<Args...>(vm, f, nargs, nkwargs, spec, /*offset=*/1);
  detail::drop_args(f, nargs, nkwargs);
  if constexpr (std::is_void_v<R>) {
    std::apply(
        [&](auto&&... a) { (self->*pmf)(std::forward<decltype(a)>(a)...); },
        tup);
    return serilang::nil;
  } else {
    R r = std::apply(
        [&](auto&&... a) {
          return (self->*pmf)(std::forward<decltype(a)>(a)...);
        },
        tup);
    serilang::TempValue tv = detail::to_value(std::move(r));
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
                       const std::vector<arg_t>* spec) {
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
                    const std::vector<arg_t>* spec) {
  return do_invoke_method(vm, f, nargs, nkwargs, self, pmf, spec);
}

}  // namespace srbind
