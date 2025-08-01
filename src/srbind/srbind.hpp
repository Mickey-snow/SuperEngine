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

#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace srbind {

using serilang::Value;

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
    if (auto ni = v.Get_if<serilang::NativeInstance>()) {
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
// named argument descriptor
// -------------------------------------------------------------
struct arg_t {
  std::string name;
  bool has_default = false;
  std::function<serilang::TempValue()> make_default;

  explicit arg_t(const char* n) : name(n) {}

  // allow: arg("x") = 42  (captures by value)
  template <class T>
  arg_t operator=(T&& v) && {
    arg_t r = *this;
    r.has_default = true;
    auto held = std::make_shared<std::decay_t<T>>(std::forward<T>(v));
    r.make_default = [held]() -> serilang::TempValue {
      return type_caster<std::decay_t<T>>::cast(*held);
    };
    return r;
  }
};

inline arg_t arg(const char* name) { return arg_t{name}; }

// -------------------------------------------------------------
// detail helpers: arg fetch & invocation
// -------------------------------------------------------------
namespace detail {

inline size_t arg_base(serilang::Fiber& f, size_t nargs, size_t nkwargs) {
  // Same layout as your deprecated NativeFunction adaptor used.
  return f.stack.size() - nkwargs * 2 - nargs;
}

inline Value& arg_at(serilang::Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     size_t i) {
  size_t base = arg_base(f, nargs, nkwargs);
  return f.stack[base + i];
}

// Pop all args (positional-only) and shrink the stack; leave callee slot in
// place.
inline void drop_args(serilang::Fiber& f, size_t nargs, size_t nkwargs) {
  (void)nkwargs;  // this version ignores kwargs
  f.stack.resize(f.stack.size() - (nkwargs * 2 + nargs));
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

// -------------------------------------------------------------

template <class T>
T from_value(Value& v) {
  return type_caster<T>::load(v);
}

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
template <class F, class... A>
serilang::NativeFunction* make_function(serilang::GarbageCollector* gc,
                                        std::string name,
                                        F&& f,
                                        A&&... a) {
  std::vector<arg_t> spec{std::forward<A>(a)...};
  return gc->Allocate<serilang::NativeFunction>(
      std::move(name),
      [fn = std::forward<F>(f), spec = std::move(spec)](
          serilang::VM& vm, serilang::Fiber& fib, uint8_t nargs,
          uint8_t nkwargs) -> Value {
        try {
          return invoke_free(vm, fib, nargs, nkwargs, fn, &spec);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        }
      });
}

template <class F>
serilang::NativeFunction* make_function(serilang::GarbageCollector* gc,
                                        std::string name,
                                        F&& f) {
  return make_function(gc, std::move(name), std::forward<F>(f),
                       std::vector<arg_t>{});
}

// SFINAE-friendly invoke for free functions
template <class F, class R, class... Args>
auto invoke_free_impl(serilang::VM& vm,
                      serilang::Fiber& f,
                      size_t nargs,
                      size_t nkwargs,
                      F&& fn,
                      R (*)(Args...),
                      const std::vector<arg_t>* spec) -> Value {
  auto tup =
      detail::load_mapped<Args...>(vm, f, nargs, nkwargs, spec, /*offset=*/0);
  detail::drop_args(f, nargs, nkwargs);
  if constexpr (std::is_void_v<R>) {
    std::apply(std::forward<F>(fn), tup);
    return serilang::nil;
  } else {
    R r = std::apply(std::forward<F>(fn), tup);
    serilang::TempValue tv = detail::to_value(std::move(r));
    return vm.gc_->TrackValue(std::move(tv));
  }
}

// helper to deduce F’s type
template <class F>
auto invoke_free(serilang::VM& vm,
                 serilang::Fiber& f,
                 size_t nargs,
                 size_t nkwargs,
                 F&& fn,
                 const std::vector<arg_t>* spec) -> Value {
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

// operator() matcher
template <class F, class C, class R, class... Args>
auto invoke_free_sig(serilang::VM& vm,
                     serilang::Fiber& f,
                     size_t nargs,
                     size_t nkwargs,
                     F&& fn,
                     R (C::*)(Args...) const,
                     const std::vector<arg_t>* spec) -> Value {
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
                     const std::vector<arg_t>* spec) -> Value {
  return invoke_free_impl(vm, f, nargs, nkwargs, std::forward<F>(fn),
                          (R (*)(Args...)) nullptr, spec);
}

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

  template <class F, class... A>
  module_& def(const char* name, F&& f, A&&... a) {
    dict_->map[name] = Value(
        make_function(gc_, name, std::forward<F>(f), std::forward<A>(a)...));
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
  static void finalize_T(void* p) { delete static_cast<T*>(p); }

 public:
  class_(module_& m, const char* name) : gc_(m.gc()) {
    cls_ = gc_->Allocate<serilang::NativeClass>();
    cls_->name = name;
    cls_->finalize = &finalize_T;
    m.dict()->map[name] = Value(cls_);
  }

  class_& no_delete() {
    cls_->finalize = nullptr;
    return *this;
  }

  // __init__(self, ...) with names/defaults
  template <class... Args, class... A>
  class_& def(init_t<Args...>, A&&... a) {
    std::vector<arg_t> spec{std::forward<A>(a)...};  // names for ctor args
    auto* nf = gc_->Allocate<serilang::NativeFunction>(
        "__init__",
        [spec = std::move(spec)](serilang::VM& vm, serilang::Fiber& f,
                                 uint8_t nargs, uint8_t nkwargs) -> Value {
          try {
            if (nargs < 1)
              throw type_error("missing 'self'");
            Value& selfv = detail::arg_at(f, nargs, nkwargs, 0);
            auto* self = selfv.Get_if<serilang::NativeInstance>();
            if (!self)
              throw type_error("self is not a native instance");
            auto tup = detail::load_mapped<Args...>(vm, f, nargs, nkwargs,
                                                    &spec, /*offset=*/1);
            detail::drop_args(f, nargs, nkwargs);
            if (self->foreign)
              throw type_error("__init__ called twice");
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

  template <class M, class... A>
  class_& def(const char* name, M pmf, A&&... a) {
    cls_->methods[name] =
        Value(make_method<T>(gc_, name, pmf, std::forward<A>(a)...));
    return *this;
  }

  template <class F, class... A>
  class_& def_static(const char* name, F&& f, A&&... a) {
    cls_->methods[name] = Value(
        make_function(gc_, name, std::forward<F>(f), std::forward<A>(a)...));
    return *this;
  }
};

}  // namespace srbind
