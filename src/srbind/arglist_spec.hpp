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
#include "utilities/mpl.hpp"

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace serilang {
class VM;
struct Fiber;
}  // namespace serilang

namespace srbind {

struct arglist_spec {
  bool has_vm = false;
  bool has_fib = false;

  uint32_t nparam = 0;
  uint32_t npos = 0;
  std::unordered_map<std::string, size_t> param_index;
  std::unordered_map<size_t, std::function<serilang::TempValue()>> defaults;
  bool has_vararg = false;
  bool has_kwarg = false;

  [[nodiscard]] std::string GetDebugString() const;
};

namespace {
// Helpers to recognize vararg/kwarg carrier types
template <class T>
inline constexpr bool is_vararg_type_v =
    std::same_as<std::remove_cvref_t<T>, std::vector<serilang::Value>>;

template <class T>
inline constexpr bool is_kwarg_type_v =
    std::same_as<std::remove_cvref_t<T>,
                 std::unordered_map<std::string, serilang::Value>>;

template <class T>
inline constexpr bool is_vm_type_v = std::same_as<T, serilang::VM&>;

template <class T>
inline constexpr bool is_fib_type_v = std::same_as<T, serilang::Fiber&>;

class spec_parser {
  bool passed_vararg = false;
  bool passed_kwarg = false;
  bool has_kwonly = false;
  arglist_spec spec;

  template <class A>
  void parse_one_arg(A&& a) {
    using T = std::decay_t<A>;

    if constexpr (std::same_as<T, arg_t>) {
      if (passed_vararg || passed_kwarg)
        throw type_error(
            "keyword argument cannot appear after var_args or kw_args");

      size_t idx = spec.param_index.size();
      auto [it, suc] = spec.param_index.try_emplace(a.name, idx);
      if (!suc)
        throw type_error("multiple keyword argument " + a.name);
      if (a.has_default)
        spec.defaults[idx] = std::move(a.make_default);

      ++spec.nparam;
      if (!a.kw_only) {
        if (has_kwonly)
          throw type_error(
              "positional arguments must appear before any keyword only "
              "argument");
        ++spec.npos;
      } else
        has_kwonly = true;
    } else if constexpr (std::same_as<T, vararg_t>) {
      if (passed_vararg)
        throw type_error("duplicate var_args");
      if (passed_kwarg)
        throw type_error("var_args must appear before kw_args");

      passed_vararg = true;
      spec.has_vararg = true;
    } else if constexpr (std::same_as<T, kwargs_t>) {
      if (passed_kwarg)
        throw type_error("duplicate kw_args");

      passed_kwarg = true;
      spec.has_kwarg = true;
    } else
      static_assert(always_false<T>);
  }

 public:
  template <class... A>
  arglist_spec parse_spec(A&&... a) {
    constexpr size_t N = sizeof...(A);
    spec.defaults.reserve(N);
    spec.param_index.reserve(N);
    (parse_one_arg(std::forward<A>(a)), ...);
    return spec;
  }
};
}  // namespace

template <argument_like... A>
arglist_spec parse_spec_impl(A&&... a) {
  return spec_parser().parse_spec(std::forward<A>(a)...);
}

template <class F>
arglist_spec parse_spec_impl() {
  using args_list =
      typename function_traits<std::remove_reference_t<F>>::argument_types;

  arglist_spec spec{};
  constexpr std::size_t N = SizeOfTypeList<args_list>::value;
  spec.nparam = static_cast<uint32_t>(N);  // start with all as positional

  if constexpr (N >= 1) {
    using first_t = typename GetNthType<0, args_list>::type;

    if constexpr (is_vm_type_v<first_t>) {
      spec.has_vm = true;
      --spec.nparam;  // drop vm from pos count
      if constexpr (N >= 2) {
        using second = typename GetNthType<1, args_list>::type;
        if constexpr (is_fib_type_v<second>) {
          spec.has_fib = true;
          --spec.nparam;
        }
      }
    } else if constexpr (is_fib_type_v<first_t>) {
      spec.has_fib = true;
      --spec.nparam;
    }
  }

  if constexpr (N > 0) {
    using last_t = typename GetNthType<N - 1, args_list>::type;

    if constexpr (is_kwarg_type_v<last_t>) {
      spec.has_kwarg = true;
      --spec.nparam;  // drop kwarg from positional count

      if constexpr (N > 1) {
        using second_last_t = typename GetNthType<N - 2, args_list>::type;
        if constexpr (is_vararg_type_v<second_last_t>) {
          spec.has_vararg = true;
          --spec.nparam;  // drop vararg from positional count
        }
      }
    } else if constexpr (is_vararg_type_v<last_t>) {
      // Only vararg present at the very end
      spec.has_vararg = true;
      --spec.nparam;
    }
  }

  // every parameter is positional
  spec.npos = spec.nparam;

  // param_index and defaults intentionally left empty
  return spec;
}

namespace helper {
template <class F>
consteval std::pair<bool, bool> deduce_vmfib() {
  using args_list =
      typename function_traits<std::remove_reference_t<F>>::argument_types;
  constexpr std::size_t args_list_size = SizeOfTypeList<args_list>::value;

  bool has_vm = false, has_fib = false;
  if constexpr (args_list_size >= 1) {
    using first_t = typename GetNthType<0, args_list>::type;
    if constexpr (is_vm_type_v<first_t>) {
      has_vm = true;
      if constexpr (args_list_size >= 2) {
        using second_t = typename GetNthType<1, args_list>::type;
        if constexpr (is_fib_type_v<second_t>)
          has_fib = true;
      }
    } else if constexpr (is_fib_type_v<first_t>)
      has_fib = true;
  }

  return std::make_pair(has_vm, has_fib);
}

}  // namespace helper

template <class F, argument_like... A>
arglist_spec parse_spec(A&&... a) {
  using args_list =
      typename function_traits<std::remove_reference_t<F>>::argument_types;
  [[maybe_unused]] constexpr std::size_t args_list_size =
      SizeOfTypeList<args_list>::value;

  constexpr std::size_t given_cnt = sizeof...(A);

  if constexpr (given_cnt == 0) {
    // deduce from callable F
    return parse_spec_impl<F>();
  } else {
    constexpr auto deduce_result = helper::deduce_vmfib<F>();
    constexpr bool has_vm = deduce_result.first, has_fib = deduce_result.second;
    static_assert(
        given_cnt + (has_vm ? 1 : 0) + (has_fib ? 1 : 0) == args_list_size,
        "argument count mismatch");

    arglist_spec spec = parse_spec_impl(std::forward<A>(a)...);
    spec.has_vm = has_vm;
    spec.has_fib = has_fib;

    return spec;
  }
}

}  // namespace srbind
