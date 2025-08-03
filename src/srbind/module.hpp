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

#include "srbind/method.hpp"
#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

namespace srbind {

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
    auto* nf = gc_->Allocate<serilang::NativeFunction>(
        "__init__",
        [spec = parse_spec(std::forward<A>(a)...)](
            serilang::VM& vm, serilang::Fiber& f, uint8_t nargs,
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
