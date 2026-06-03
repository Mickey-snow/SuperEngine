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

#include <functional>

namespace srbind {

namespace detail {

template <class T, class U>
inline constexpr bool is_method_self_arg_v =
    std::is_pointer_v<std::remove_cv_t<U>> &&
    std::same_as<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<U>>>,
                 std::remove_cv_t<T>>;

template <class R, class ArgsList>
struct function_type_from_args;

template <class R, class... Args>
struct function_type_from_args<R, TypeList<Args...>> {
  using type = R(Args...);
};

template <class T, class F>
struct self_first_signature {
  using traits = function_traits<std::remove_reference_t<F>>;
  using args_list = typename traits::argument_types;
  static constexpr std::size_t nargs = SizeOfTypeList<args_list>::value;

  static_assert(nargs > 0,
                "free function methods must take T* self as first argument");

  using self_t = typename GetNthType<0, args_list>::type;
  static_assert(is_method_self_arg_v<T, self_t>,
                "free function methods must take T* self as first argument");

  using rest_args = typename Slice<1, nargs - 1, args_list>::type;
  using type = typename function_type_from_args<typename traits::result_type,
                                                rest_args>::type;
};

template <class T, class M, class... A>
arglist_spec parse_method_spec(A&&... a) {
  using method_t = std::remove_reference_t<M>;
  if constexpr (std::is_member_function_pointer_v<method_t>) {
    return parse_spec<M>(std::forward<A>(a)...);
  } else {
    using sig = typename self_first_signature<T, M>::type;
    return parse_spec<sig>(std::forward<A>(a)...);
  }
}

template <class T, class R, class... Args, class... LoadArgs, class... Extra>
auto invoke_method_extra_impl(serilang::VM& vm,
                              serilang::Fiber& f,
                              size_t nargs,
                              size_t nkwargs,
                              T* self,
                              R (T::*pmf)(Args...),
                              R (*)(LoadArgs...),
                              std::tuple<Extra...> extras,
                              const arglist_spec& spec) -> serilang::TempValue {
  auto tup = std::tuple_cat(
      std::move(extras),
      load_args<LoadArgs...>(f.op_stack, nargs - 1, nkwargs, spec));
  f.op_stack.pop_back();  // self

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
    return tv;
  }
}

template <class T, class R, class... Args>
auto invoke_method_extra(serilang::VM& vm,
                         serilang::Fiber& f,
                         size_t nargs,
                         size_t nkwargs,
                         T* self,
                         R (T::*pmf)(Args...),
                         const arglist_spec& spec) -> serilang::TempValue {
  return invoke_method_extra_impl(vm, f, nargs, nkwargs, self, pmf,
                                  (R (*)(Args...)) nullptr, std::make_tuple(),
                                  spec);
}

template <class T, class R, std::same_as<serilang::VM&> vm_t, class... Args>
auto invoke_method_extra(serilang::VM& vm,
                         serilang::Fiber& f,
                         size_t nargs,
                         size_t nkwargs,
                         T* self,
                         R (T::*pmf)(vm_t, Args...),
                         const arglist_spec& spec) -> serilang::TempValue {
  if (!spec.has_vm)
    throw type_error("Unexpected vm argument");
  return invoke_method_extra_impl(vm, f, nargs, nkwargs, self, pmf,
                                  (R (*)(Args...)) nullptr,
                                  std::make_tuple(std::ref(vm)), spec);
}

template <class T, class R, std::same_as<serilang::Fiber&> fib_t, class... Args>
auto invoke_method_extra(serilang::VM& vm,
                         serilang::Fiber& f,
                         size_t nargs,
                         size_t nkwargs,
                         T* self,
                         R (T::*pmf)(fib_t, Args...),
                         const arglist_spec& spec) -> serilang::TempValue {
  if (!spec.has_fib)
    throw type_error("Unexpected fiber argument");
  return invoke_method_extra_impl(vm, f, nargs, nkwargs, self, pmf,
                                  (R (*)(Args...)) nullptr,
                                  std::make_tuple(std::ref(f)), spec);
}

template <class T,
          class R,
          std::same_as<serilang::VM&> vm_t,
          std::same_as<serilang::Fiber&> fib_t,
          class... Args>
auto invoke_method_extra(serilang::VM& vm,
                         serilang::Fiber& f,
                         size_t nargs,
                         size_t nkwargs,
                         T* self,
                         R (T::*pmf)(vm_t, fib_t, Args...),
                         const arglist_spec& spec) -> serilang::TempValue {
  if (!spec.has_vm || !spec.has_fib)
    throw type_error("Unexpected vm and fiber arguments");
  return invoke_method_extra_impl(
      vm, f, nargs, nkwargs, self, pmf, (R (*)(Args...)) nullptr,
      std::make_tuple(std::ref(vm), std::ref(f)), spec);
}

template <class T,
          class F,
          class Self,
          class R,
          class... Args,
          class... LoadArgs,
          class... Extra>
auto invoke_self_function_extra_impl(serilang::VM& vm,
                                     serilang::Fiber& f,
                                     size_t nargs,
                                     size_t nkwargs,
                                     T* self,
                                     F&& fn,
                                     R (*)(Self, Args...),
                                     R (*)(LoadArgs...),
                                     std::tuple<Extra...> extras,
                                     const arglist_spec& spec)
    -> serilang::TempValue {
  static_assert(is_method_self_arg_v<T, Self>,
                "free function methods must take T* self as first argument");

  auto tup = std::tuple_cat(
      std::move(extras),
      load_args<LoadArgs...>(f.op_stack, nargs - 1, nkwargs, spec));
  f.op_stack.pop_back();  // self

  if constexpr (std::is_void_v<R>) {
    std::apply(
        [&](auto&&... a) {
          std::invoke(std::forward<F>(fn), static_cast<Self>(self),
                      std::forward<decltype(a)>(a)...);
        },
        std::move(tup));
    return serilang::nil;
  } else {
    R r = std::apply(
        [&](auto&&... a) {
          return std::invoke(std::forward<F>(fn), static_cast<Self>(self),
                             std::forward<decltype(a)>(a)...);
        },
        std::move(tup));
    serilang::TempValue tv = type_caster<std::decay_t<R>>::cast(std::move(r));
    return tv;
  }
}

template <class T, class F, class Self, class R, class... Args>
auto invoke_self_function_extra(serilang::VM& vm,
                                serilang::Fiber& f,
                                size_t nargs,
                                size_t nkwargs,
                                T* self,
                                F&& fn,
                                R (*sig)(Self, Args...),
                                const arglist_spec& spec)
    -> serilang::TempValue {
  return invoke_self_function_extra_impl(
      vm, f, nargs, nkwargs, self, std::forward<F>(fn), sig,
      (R (*)(Args...)) nullptr, std::make_tuple(), spec);
}

template <class T,
          class F,
          class Self,
          class R,
          std::same_as<serilang::VM&> vm_t,
          class... Args>
auto invoke_self_function_extra(serilang::VM& vm,
                                serilang::Fiber& f,
                                size_t nargs,
                                size_t nkwargs,
                                T* self,
                                F&& fn,
                                R (*sig)(Self, vm_t, Args...),
                                const arglist_spec& spec)
    -> serilang::TempValue {
  if (!spec.has_vm)
    throw type_error("Unexpected vm argument");
  return invoke_self_function_extra_impl(
      vm, f, nargs, nkwargs, self, std::forward<F>(fn), sig,
      (R (*)(Args...)) nullptr, std::make_tuple(std::ref(vm)), spec);
}

template <class T,
          class F,
          class Self,
          class R,
          std::same_as<serilang::Fiber&> fib_t,
          class... Args>
auto invoke_self_function_extra(serilang::VM& vm,
                                serilang::Fiber& f,
                                size_t nargs,
                                size_t nkwargs,
                                T* self,
                                F&& fn,
                                R (*sig)(Self, fib_t, Args...),
                                const arglist_spec& spec)
    -> serilang::TempValue {
  if (!spec.has_fib)
    throw type_error("Unexpected fiber argument");
  return invoke_self_function_extra_impl(
      vm, f, nargs, nkwargs, self, std::forward<F>(fn), sig,
      (R (*)(Args...)) nullptr, std::make_tuple(std::ref(f)), spec);
}

template <class T,
          class F,
          class Self,
          class R,
          std::same_as<serilang::VM&> vm_t,
          std::same_as<serilang::Fiber&> fib_t,
          class... Args>
auto invoke_self_function_extra(serilang::VM& vm,
                                serilang::Fiber& f,
                                size_t nargs,
                                size_t nkwargs,
                                T* self,
                                F&& fn,
                                R (*sig)(Self, vm_t, fib_t, Args...),
                                const arglist_spec& spec)
    -> serilang::TempValue {
  if (!spec.has_vm || !spec.has_fib)
    throw type_error("Unexpected vm and fiber arguments");
  return invoke_self_function_extra_impl(
      vm, f, nargs, nkwargs, self, std::forward<F>(fn), sig,
      (R (*)(Args...)) nullptr, std::make_tuple(std::ref(vm), std::ref(f)),
      spec);
}

template <class T, class F, class C, class R, class... Args>
auto invoke_self_function_sig(serilang::VM& vm,
                              serilang::Fiber& f,
                              size_t nargs,
                              size_t nkwargs,
                              T* self,
                              F&& fn,
                              R (C::*)(Args...) const,
                              const arglist_spec& spec) -> serilang::TempValue {
  return invoke_self_function_extra(vm, f, nargs, nkwargs, self,
                                    std::forward<F>(fn),
                                    (R (*)(Args...)) nullptr, spec);
}

template <class T, class F, class C, class R, class... Args>
auto invoke_self_function_sig(serilang::VM& vm,
                              serilang::Fiber& f,
                              size_t nargs,
                              size_t nkwargs,
                              T* self,
                              F&& fn,
                              R (C::*)(Args...),
                              const arglist_spec& spec) -> serilang::TempValue {
  return invoke_self_function_extra(vm, f, nargs, nkwargs, self,
                                    std::forward<F>(fn),
                                    (R (*)(Args...)) nullptr, spec);
}

template <class T, class F>
auto invoke_self_function(serilang::VM& vm,
                          serilang::Fiber& f,
                          size_t nargs,
                          size_t nkwargs,
                          T* self,
                          F&& fn,
                          const arglist_spec& spec) -> serilang::TempValue {
  using Fn = std::decay_t<F>;

  if constexpr (std::is_function_v<Fn>) {
    using Sig = Fn;
    return invoke_self_function_extra(vm, f, nargs, nkwargs, self,
                                      std::forward<F>(fn),
                                      static_cast<Sig*>(nullptr), spec);
  } else if constexpr (std::is_pointer_v<Fn> &&
                       std::is_function_v<std::remove_pointer_t<Fn>>) {
    using Sig = std::remove_pointer_t<Fn>;
    return invoke_self_function_extra(vm, f, nargs, nkwargs, self,
                                      std::forward<F>(fn),
                                      static_cast<Sig*>(nullptr), spec);
  } else {
    using Sig = decltype(&Fn::operator());
    return invoke_self_function_sig(vm, f, nargs, nkwargs, self,
                                    std::forward<F>(fn), Sig{}, spec);
  }
}

// const member method
template <class T, class R, class... Args>
auto do_invoke_method(serilang::VM& vm,
                      serilang::Fiber& f,
                      size_t nargs,
                      size_t nkwargs,
                      T* self,
                      R (T::*pmf)(Args...) const,
                      const arglist_spec& spec) -> serilang::TempValue {
  return invoke_method_extra(vm, f, nargs, nkwargs, self,
                             (R (T::*)(Args...))pmf, spec);
}

// non-const member method
template <class T, class R, class... Args>
auto do_invoke_method(serilang::VM& vm,
                      serilang::Fiber& f,
                      size_t nargs,
                      size_t nkwargs,
                      T* self,
                      R (T::*pmf)(Args...),
                      const arglist_spec& spec) -> serilang::TempValue {
  return invoke_method_extra(vm, f, nargs, nkwargs, self, pmf, spec);
}

template <class T, class M>
auto invoke_method(serilang::VM& vm,
                   serilang::Fiber& f,
                   size_t nargs,
                   size_t nkwargs,
                   T* self,
                   M pmf,
                   const arglist_spec& spec) -> serilang::TempValue {
  if constexpr (std::is_member_function_pointer_v<M>) {
    return do_invoke_method(vm, f, nargs, nkwargs, self, pmf, spec);
  } else {
    return invoke_self_function(vm, f, nargs, nkwargs, self, pmf, spec);
  }
}
}  // namespace detail

// -------------------------------------------------------------
// wrap method: def("name", &T::method, arg("x")=0, ...)
// Also accepts free callables with T* self as the first parameter.
// BoundMethod inserts receiver as arg0; we convert it to T*
// -------------------------------------------------------------
template <class T, class M>
serilang::NativeFunction* make_method(serilang::GarbageCollector* gc,
                                      std::string name,
                                      M method,
                                      arglist_spec spec) {
  return gc->Allocate<serilang::NativeFunction>(
      std::move(name),
      [method, spec = std::move(spec)](serilang::VM& vm, serilang::Fiber& f,
                                       uint8_t nargs,
                                       uint8_t nkwargs) -> serilang::TempValue {
        try {
          if (nargs == 0)
            throw type_error("missing 'self'");
          Value& selfv = f.op_stack.end()[-nargs - 2 * nkwargs];
          T* self = type_caster<T*>::load(selfv);
          return detail::invoke_method(vm, f, nargs, nkwargs, self, method,
                                       spec);
        } catch (const type_error& e) {
          throw serilang::RuntimeError(e.what());
        } catch (const std::exception& e) {
          throw serilang::RuntimeError(e.what());
        } catch (...) {
          throw serilang::RuntimeError("uncaught error");
        }
      });
}

}  // namespace srbind
