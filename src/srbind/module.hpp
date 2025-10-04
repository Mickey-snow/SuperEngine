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
#include "srbind/method.hpp"
#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

namespace srbind {

// -------------------------------------------------------------
// module: where we register functions/classes
// -------------------------------------------------------------
class module_ {
  serilang::GarbageCollector* gc_;
  serilang::Dict* dict_;

 public:
  module_(serilang::GarbageCollector* gc, serilang::Dict* dict)
      : gc_(gc), dict_(dict) {}
  module_(serilang::VM& vm, char const* name) : gc_(vm.gc_.get()) {
    dict_ = gc_->Allocate<serilang::Dict>();
    serilang::Module* mod = gc_->Allocate<serilang::Module>(name, dict_);
    vm.builtins_->map[name] = Value(mod);
  }

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
// init helpers -> binds __init__ to construct T inside NativeInstance
// -------------------------------------------------------------
template <class... Args>
struct init_t {};

// usage: class_<T>(...).def(init<Args...>(), arg("x")=..., ...)
template <class... Args>
constexpr init_t<Args...> init() {
  return {};
}

template <class F>
struct init_factory_t {
  F factory;
};

// usage: class_<T>(...).def(init([](Args...){...}), arg("x")=..., ...)
template <class F>
constexpr init_factory_t<std::decay_t<F>> init(F&& f) {
  return {std::forward<F>(f)};
}

namespace detail {
// pointer conversion helper
template <class T, class R>
static T* convert_factory_return(R&& r) {
  using RD = std::decay_t<R>;
  if constexpr (std::is_pointer_v<RD>) {
    using Pointee = std::remove_pointer_t<RD>;
    static_assert(
        std::is_base_of_v<T, Pointee>,
        "Factory must return (derived) T* or std::unique_ptr<derived T>");
    return r;
  } else if constexpr (is_unique_ptr_v<RD>) {
    using E = typename RD::element_type;
    static_assert(std::is_base_of_v<T, E>,
                  "unique_ptr element must be T or derived from T");
    static_assert(std::is_same_v<typename is_unique_ptr<RD>::deleter_type,
                                 std::default_delete<E>>,
                  "unique_ptr deleter must be std::default_delete");

    return r.release();  // transfer ownership to engine (deleted by finalize)
  } else {
    static_assert(
        !sizeof(RD),
        "Unsupported factory return type. Use T* or std::unique_ptr<T>.");
    return nullptr;
  }
}
}  // namespace detail

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

  // __init__ via operator new
  template <class... Args, class... A>
  class_& def(init_t<Args...>, A&&... a) {
    arglist_spec spec = parse_spec<T*(Args...)>(std::forward<A>(a)...);

    auto* nf = gc_->Allocate<serilang::NativeFunction>(
        "__init__",
        [spec = std::move(spec)](serilang::VM& vm, serilang::Fiber& f,
                                 uint8_t nargs,
                                 uint8_t nkwargs) -> serilang::TempValue {
          try {
            if (nargs < 1)
              throw type_error("missing 'self'");
            Value selfv = std::move(f.stack.end()[-nargs - 2 * nkwargs]);
            auto* self = selfv.Get_if<serilang::NativeInstance>();
            if (!self)
              throw type_error("self is not a native instance");
            auto tup = load_args<Args...>(f.stack, nargs - 1, nkwargs, spec);
            f.stack.pop_back();  // self

            if (self->foreign)
              throw type_error("__init__ called twice");
            T* obj = std::apply(
                [](auto&&... a) {
                  return new T(std::forward<decltype(a)>(a)...);
                },
                std::move(tup));
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

  // __init__ via native factory
  // Factory signature:  R(Args...) where R is T* or std::unique_ptr<T/Derived>
  template <class F, class... A>
  class_& def(init_factory_t<F> tag, A&&... a) {
    arglist_spec spec = parse_spec<F>(std::forward<A>(a)...);

    auto* nf = gc_->Allocate<serilang::NativeFunction>(
        "__init__",
        [factory = std::move(tag.factory), spec = std::move(spec)](
            serilang::VM& vm, serilang::Fiber& fib, uint8_t nargs,
            uint8_t nkwargs) -> serilang::TempValue {
          try {
            if (nargs < 1)
              throw type_error("missing 'self'");
            Value selfv = std::move(fib.stack.end()[-nargs - 2 * nkwargs]);
            auto* self = selfv.Get_if<serilang::NativeInstance>();
            if (!self)
              throw type_error("self is not a native instance");
            if (self->foreign)
              throw type_error("__init__ called twice");

            auto r =
                detail::invoke_free(vm, fib, nargs - 1, nkwargs, factory, spec);
            fib.stack.pop_back();  // self
            T* raw = detail::convert_factory_return<T>(std::move(r));
            if (!raw)
              throw type_error("factory returned null");
            self->SetForeign<T>(raw);

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
    arglist_spec spec = parse_spec<M>(std::forward<A>(a)...);

    cls_->methods[name] =
        Value(make_method<T>(gc_, name, pmf, std::move(spec)));
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
