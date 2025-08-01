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

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

namespace srbind {

using serilang::Dict;
using serilang::Fiber;
using serilang::List;
using serilang::NativeClass;
using serilang::NativeFunction;
using serilang::NativeInstance;
using serilang::Value;
using serilang::VM;

struct type_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// -------------------------------------------------------------
// type_caster<T>
// -------------------------------------------------------------
template <class T, class Enable = void>
struct type_caster;

// primitives
template <>
struct type_caster<int> {
  static int load(Value& v) {
    if (auto p = v.Get_if<int>())
      return *p;
    throw type_error("expected int, got " + v.Desc());
  }
  static serilang::TempValue cast(int x) { return Value(x); }
};
template <>
struct type_caster<double> {
  static double load(Value& v) {
    if (auto p = v.Get_if<double>())
      return *p;
    if (auto pi = v.Get_if<int>())
      return static_cast<double>(*pi);
    throw type_error("expected double, got " + v.Desc());
  }
  static serilang::TempValue cast(double x) { return Value(x); }
};
template <>
struct type_caster<bool> {
  static bool load(Value& v) {
    if (auto p = v.Get_if<bool>())
      return *p;
    throw type_error("expected bool, got " + v.Desc());
  }
  static serilang::TempValue cast(bool x) { return Value(x); }
};
template <>
struct type_caster<std::string> {
  static std::string load(Value& v) {
    if (auto p = v.Get_if<std::string>())
      return *p;
    throw type_error("expected str, got " + v.Desc());
  }
  static serilang::TempValue cast(std::string s) { return Value(std::move(s)); }
};

// pass-through Value
template <>
struct type_caster<Value> {
  static Value& load(Value& v) { return v; }
  static serilang::TempValue cast(const Value& v) { return v; }
};

// pointer to bound native instance foreign
template <class T>
struct type_caster<T*, std::enable_if_t<!std::is_void_v<T>>> {
  static T* load(Value& v) {
    if (auto ni = v.Get_if<NativeInstance>()) {
      T* p = ni->GetForeign<T>();
      if (!p)
        throw type_error("null native instance for requested type");
      return p;
    }
    throw type_error("expected native instance for pointer type, got " +
                     v.Desc());
  }
  // By default, we don't auto-wrap raw T* back into a NativeInstance
  // (doing so requires class registry). Encourage returning
  // Value/void/primitives.
};

// -------------------------------------------------------------
// detail helpers: arg fetch & invocation
// -------------------------------------------------------------
namespace detail {

inline size_t arg_base(Fiber& f, size_t nargs, size_t nkwargs) {
  // Same layout as your deprecated NativeFunction adaptor used.
  return f.stack.size() - nkwargs * 2 - nargs;
}

inline Value& arg_at(Fiber& f, size_t nargs, size_t nkwargs, size_t i) {
  size_t base = arg_base(f, nargs, nkwargs);
  return f.stack[base + i];
}

// Pop all args (positional-only) and shrink the stack; leave callee slot in
// place.
inline void drop_args(Fiber& f, size_t nargs, size_t nkwargs) {
  (void)nkwargs;  // this version ignores kwargs
  f.stack.resize(f.stack.size() - (nkwargs * 2 + nargs));
}

template <class T>
T from_value(Value& v) {
  return type_caster<T>::load(v);
}

template <class... Args, size_t... I>
std::tuple<Args...> load_positional(Fiber& f,
                                    size_t nargs,
                                    size_t nkwargs,
                                    std::index_sequence<I...>) {
  if (sizeof...(Args) != nargs)
    throw type_error("expected " + std::to_string(sizeof...(Args)) +
                     " positional args, got " + std::to_string(nargs));
  return std::tuple<Args...>{from_value<Args>(arg_at(f, nargs, nkwargs, I))...};
}

template <class R>
serilang::TempValue to_value(R&& r) {
  return type_caster<std::decay_t<R>>::cast(std::forward<R>(r));
}
inline serilang::Value to_value(void) { return serilang::nil; }

template <class F, class Tuple, size_t... I>
auto apply(F&& f, Tuple&& t, std::index_sequence<I...>) {
  return std::invoke(std::forward<F>(f),
                     std::get<I>(std::forward<Tuple>(t))...);
}

}  // namespace detail

// -------------------------------------------------------------
// wrap free function: def("name", callable)
// positional-only for now
// -------------------------------------------------------------
template <class F>
NativeFunction* make_function(serilang::GarbageCollector* gc,
                              std::string name,
                              F&& f) {
  // Instead of signature detection gymnastics, specialize with helper below:
  return gc->Allocate<NativeFunction>(
      std::move(name),
      [fn = std::forward<F>(f)](VM& vm, Fiber& f, uint8_t nargs,
                                uint8_t nkwargs) -> Value {
        try {
          return invoke_free(vm, f, nargs, nkwargs, fn);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

// SFINAE-friendly invoke for free functions
template <class F, class R, class... Args>
auto invoke_free_impl(VM& vm,
                      Fiber& f,
                      size_t nargs,
                      size_t nkwargs,
                      F&& fn,
                      R (*)(Args...)) -> Value {
  (void)vm;
  auto tup = detail::load_positional<Args...>(
      f, nargs, nkwargs, std::index_sequence_for<Args...>{});
  detail::drop_args(f, nargs, nkwargs);
  if constexpr (std::is_void_v<R>) {
    detail::apply(std::forward<F>(fn), tup, std::index_sequence_for<Args...>{});
    return serilang::nil;
  } else {
    R r = detail::apply(std::forward<F>(fn), tup,
                        std::index_sequence_for<Args...>{});
    serilang::TempValue val = detail::to_value(std::move(r));
    return vm.gc_->TrackValue(std::move(val));
  }
}

// helper to deduce F’s type
template <class F>
auto invoke_free(VM& vm, Fiber& f, size_t nargs, size_t nkwargs, F&& fn)
    -> Value {
  using Fn = std::decay_t<F>;
  if constexpr (std::is_function_v<Fn>) {
    using R = std::invoke_result_t<Fn>;
    return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                            (R (*)(void)) nullptr);
  } else if constexpr (std::is_pointer_v<Fn> &&
                       std::is_function_v<std::remove_pointer_t<Fn>>) {
    using R = std::invoke_result_t<Fn>;
    return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                            (R (*)(void)) nullptr);
  } else {
    // Functor or lambda
    using Sig = decltype(&Fn::operator());
    return invoke_free_sig(vm, f, nargs, nkwargs, std::forward<F>(fn), Sig{});
  }
}

// operator() matcher
template <class F, class C, class R, class... Args>
auto invoke_free_sig(VM& vm,
                     Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     F&& fn,
                     R (C::*)(Args...) const) -> Value {
  return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                          (R (*)(Args...)) nullptr);
}

template <class F, class C, class R, class... Args>
auto invoke_free_sig(VM& vm,
                     Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     F&& fn,
                     R (C::*)(Args...)) -> Value {
  return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                          (R (*)(Args...)) nullptr);
}

// -------------------------------------------------------------
// wrap member function: def("name", &T::method)
// BoundMethod inserts receiver as arg0; we convert it to T*
// -------------------------------------------------------------
template <class T, class M>
NativeFunction* make_method(serilang::GarbageCollector* gc,
                            std::string name,
                            M method) {
  return gc->Allocate<NativeFunction>(
      std::move(name),
      [method](VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) -> Value {
        try {
          // Expect at least self
          if (nargs == 0)
            throw type_error("missing 'self'");
          // load self (arg0)
          Value& selfv = detail::arg_at(f, nargs, nkwargs, 0);
          T* self = type_caster<T*>::load(selfv);

          // remaining args
          return invoke_method(vm, f, nargs, nkwargs, self, method);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

template <class T, class R, class... Args>
Value do_invoke_method(VM& vm,
                       Fiber& f,
                       size_t nargs,
                       size_t nkwargs,
                       T* self,
                       R (T::*pmf)(Args...)) {
  // args after self
  if (nargs != sizeof...(Args) + 1)
    throw type_error("expected " + std::to_string(sizeof...(Args)) +
                     " args after self");
  auto tup = detail::load_positional<Args...>(
      f, nargs - 1, nkwargs, std::index_sequence_for<Args...>{});
  detail::drop_args(f, nargs, nkwargs);
  if constexpr (std::is_void_v<R>) {
    std::apply([&](Args&&... a) { (self->*pmf)(std::forward<Args>(a)...); },
               tup);
    return serilang::nil;
  } else {
    R r = std::apply(
        [&](Args&&... a) { return (self->*pmf)(std::forward<Args>(a)...); },
        tup);
    serilang::TempValue val = detail::to_value(std::move(r));
    return vm.gc_->TrackValue(std::move(val));
  }
}

template <class T, class R, class... Args>
Value do_invoke_method(VM& vm,
                       Fiber& f,
                       size_t nargs,
                       size_t nkwargs,
                       T* self,
                       R (T::*pmf)(Args...) const) {
  return do_invoke_method(vm, f, nargs, nkwargs, self, (R (T::*)(Args...))pmf);
}

// overload selector
template <class T, class M>
Value invoke_method(VM& vm,
                    Fiber& f,
                    size_t nargs,
                    size_t nkwargs,
                    T* self,
                    M pmf) {
  return do_invoke_method(vm, f, nargs, nkwargs, self, pmf);
}

// -------------------------------------------------------------
// init<T(Args...)>() -> binds __init__ to construct T inside NativeInstance
// -------------------------------------------------------------
template <class... Args>
struct init_t {};

template <class... Args>
constexpr init_t<Args...> init() {
  return {};
}

// -------------------------------------------------------------
// module: where we register functions/classes
// -------------------------------------------------------------
class module_ {
  serilang::GarbageCollector* gc_;
  serilang::Dict* dict_;

 public:
  module_(serilang::GarbageCollector* gc, serilang::Dict* dict)
      : gc_(gc), dict_(dict) {}

  template <class F>
  module_& def(const char* name, F&& f) {
    dict_->map[name] = Value(gc_->Allocate<NativeFunction>(
        name,
        [fn = std::forward<F>(f)](VM& vm, Fiber& f, uint8_t nargs,
                                  uint8_t nkwargs) -> Value {
          try {
            return invoke_free(vm, f, nargs, nkwargs, fn);
          } catch (const type_error& e) {
            throw serilang::RuntimeError(e.what());
          } catch (const std::exception& e) {
            throw serilang::RuntimeError(e.what());
          }
        }));
    return *this;
  }

  serilang::Dict* dict() const { return dict_; }
  serilang::GarbageCollector* gc() const { return gc_; }
};

// -------------------------------------------------------------
// class_<T>: binds a NativeClass container + methods + __init__
// -------------------------------------------------------------
template <class T>
class class_ {
  serilang::GarbageCollector* gc_;
  serilang::NativeClass* cls_;
  bool has_finalizer_ = false;

  // finalizer for T
  static void finalize_T(void* p) { delete static_cast<T*>(p); }

 public:
  class_(module_& m, const char* name) : gc_(m.gc()) {
    cls_ = gc_->Allocate<NativeClass>();
    cls_->name = name;
    // default finalizer (opt-out if user calls .no_delete())
    cls_->finalize = &finalize_T;
    has_finalizer_ = true;

    // expose the class constructor (allocates instance only; user calls
    // __init__)
    m.dict()->map[name] = Value(cls_);
  }

  // disable automatic deletion (user manages lifetime)
  class_& no_delete() {
    cls_->finalize = nullptr;
    has_finalizer_ = false;
    return *this;
  }

  // __init__ binding: constructs T and stores into NativeInstance::foreign
  template <class... Args>
  class_& def(init_t<Args...>) {
    auto* nf = gc_->Allocate<NativeFunction>(
        "__init__",
        [](VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) -> Value {
          try {
            if (nargs < 1)
              throw type_error("missing 'self'");
            // self at arg0
            Value& selfv = detail::arg_at(f, nargs, nkwargs, 0);
            auto* self = selfv.Get_if<NativeInstance>();
            if (!self)
              throw type_error("self is not a native instance");
            // load C++ args (after self)
            auto tup = detail::load_positional<Args...>(
                f, nargs - 1, nkwargs, std::index_sequence_for<Args...>{});
            detail::drop_args(f, nargs, nkwargs);

            // guard: if already has foreign, prevent double init
            if (self->foreign)
              throw type_error("__init__ called twice");

            // construct T
            T* obj = std::apply(
                [](auto&&... a) {
                  return new T(std::forward<decltype(a)>(a)...);
                },
                tup);
            self->SetForeign<T>(obj);
            return serilang::nil;
          } catch (const type_error& e) {
            throw serilang::RuntimeError(e.what());
          } catch (const std::exception& e) {
            throw serilang::RuntimeError(e.what());
          }
        });
    cls_->methods["__init__"] = Value(nf);
    return *this;
  }

  // bind a member function
  template <class M>
  class_& def(const char* name, M pmf) {
    cls_->methods[name] = Value(make_method<T>(gc_, name, pmf));
    return *this;
  }

  // bind a const free-function as static method (no self)
  template <class F>
  class_& def_static(const char* name, F&& f) {
    cls_->methods[name] = Value(make_function(gc_, name, std::forward<F>(f)));
    return *this;
  }

  // simple read/write field via getter/setter lambdas (syntactic sugar)
  template <class Getter, class Setter>
  class_& def_property(const char* name, Getter get, Setter set) {
    // generate name() and set_name(v)
    std::string gname = name;
    std::string sname = std::string("set_") + name;
    def(gname.c_str(), get);
    def(sname.c_str(), set);
    return *this;
  }

  NativeClass* get() const { return cls_; }
};

}  // namespace srbind
