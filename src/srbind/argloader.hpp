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
#include "srbind/args.hpp"
#include "srbind/detail.hpp"
#include "vm/value.hpp"

#include <format>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace srbind {

struct argloader_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct argloader_internal_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace {

template <typename typelist, size_t... I>
inline auto cast_args_impl(std::vector<Value> vals,
                           std::index_sequence<I...> /*unused*/
                           ) -> typename Unpack<std::tuple, typelist>::type {
  return std::make_tuple(
      type_caster<typename GetNthType<I, typelist>::type>::load(vals.at(I))...);
}

template <typename typelist>
auto load_args_impl(std::vector<Value>& stack,
                    size_t nargs,
                    size_t nkwargs,
                    const arglist_spec& spec)
    -> std::tuple<typename Unpack<std::tuple, typelist>::type,
                  std::vector<Value>,
                  std::unordered_map<std::string, Value>> {
  constexpr size_t N = SizeOfTypeList<typelist>::value;

  if (nargs > spec.nparam && !spec.has_vararg)
    throw argloader_error("too many arguments");

  const auto base = stack.size() - nargs - 2 * nkwargs;
  if (stack.size() < nargs + 2 * nkwargs) {
    throw argloader_error("stack underflow for given nargs/nkwargs");
  }

  std::vector<Value> posargs(
      std::make_move_iterator(stack.begin() + base),
      std::make_move_iterator(stack.begin() + base + nargs));
  posargs.reserve(std::max<size_t>(nargs, spec.nparam));
  std::unordered_map<std::string, Value> kwargs;
  kwargs.reserve(nkwargs);
  size_t idx = base + nargs;
  for (size_t i = 0; i < nkwargs; ++i) {
    std::string* k = stack[idx++].Get_if<std::string>();
    Value v = std::move(stack[idx++]);
    auto [it, ok] = kwargs.try_emplace(std::move(*k), std::move(v));
    if (!ok)
      throw argloader_error(std::format("duplicate keyword '{}'", it->first));
  }
  stack.resize(base);

  std::vector<Value> finalargs(spec.nparam);
  std::vector<bool> assigned(spec.nparam, false);
  std::vector<Value> rest;
  std::unordered_map<std::string, Value> extra_kwargs;

  std::move(posargs.begin(),
            posargs.begin() + std::min<size_t>(posargs.size(), spec.npos),
            finalargs.begin());
  std::fill(assigned.begin(),
            assigned.begin() + std::min<size_t>(posargs.size(), spec.npos),
            true);
  if (posargs.size() > spec.npos) {
    rest.assign(std::make_move_iterator(posargs.begin() + spec.npos),
                std::make_move_iterator(posargs.end()));
  }

  for (auto& [k, v] : kwargs) {
    auto it = spec.param_index.find(k);
    if (it != spec.param_index.end()) {
      const size_t idx = it->second;
      if (assigned[idx])
        throw argloader_error(
            std::format("multiple values for argument '{}'", k));
      finalargs[idx] = std::move(v);
      assigned[idx] = true;
    }

    else
      extra_kwargs.emplace(std::move(k), std::move(v));
  }

  for (size_t i = 0; i < spec.nparam; ++i) {
    if (!assigned[i]) {
      const auto it = spec.defaults.find(i);
      if (it == spec.defaults.cend())
        throw argloader_error(std::format("missing argument #{}", i));
      finalargs[i] = std::get<Value>(std::invoke(it->second));
    }
  }

  if (!spec.has_vararg && !rest.empty())
    throw argloader_error("too many arguments");
  if (!spec.has_kwarg && !extra_kwargs.empty())
    throw argloader_error("too many arguments");

  return std::make_tuple(
      cast_args_impl<typelist>(std::move(finalargs),
                               std::make_index_sequence<N>{}),
      std::move(rest), std::move(extra_kwargs));
}
}  // namespace

template <class... Args>
auto load_args(std::vector<Value>& stack,
               size_t nargs,
               size_t nkwargs,
               const arglist_spec& spec) -> std::tuple<Args...> {
  using arglist = TypeList<Args...>;
  constexpr size_t N = sizeof...(Args);

  using VT = std::vector<Value>;
  using KWT = std::unordered_map<std::string, Value>;

  static const auto internal_error =
      argloader_internal_error("cannot load arguments");
  static const auto slotmissing_error = argloader_error(
      "cannot load arguments: vararg/kwarg present but not requested by "
      "signature");

  if constexpr (N >= 2) {
    if (spec.has_vararg && spec.has_kwarg) {
      if constexpr (std::convertible_to<
                        VT, typename GetNthType<N - 2, arglist>::type> &&
                    std::convertible_to<
                        KWT, typename GetNthType<N - 1, arglist>::type>) {
        auto [r, rest, extra_kwargs] =
            load_args_impl<typename Slice<0, N - 2, arglist>::type>(
                stack, nargs, nkwargs, spec);
        return std::tuple_cat(std::move(r), std::tuple<VT>{std::move(rest)},
                              std::tuple<KWT>{std::move(extra_kwargs)});
      } else
        throw internal_error;
    } else if (spec.has_vararg) {
      if constexpr (std::convertible_to<
                        VT, typename GetNthType<N - 1, arglist>::type>) {
        auto [r, rest, extra_kwargs] =
            load_args_impl<typename Slice<0, N - 1, arglist>::type>(
                stack, nargs, nkwargs, spec);
        return std::tuple_cat(std::move(r), std::tuple<VT>{std::move(rest)});
      } else
        throw internal_error;
    } else if (spec.has_kwarg) {
      if constexpr (std::convertible_to<
                        KWT, typename GetNthType<N - 1, arglist>::type>) {
        auto [r, rest, extra_kwargs] =
            load_args_impl<typename Slice<0, N - 1, arglist>::type>(
                stack, nargs, nkwargs, spec);
        return std::tuple_cat(std::move(r),
                              std::tuple<KWT>{std::move(extra_kwargs)});

      } else
        throw internal_error;
    }
  }

  else if constexpr (N >= 1) {
    if (spec.has_vararg && spec.has_kwarg)
      throw slotmissing_error;

    if (spec.has_vararg) {
      if constexpr (std::convertible_to<
                        VT, typename GetNthType<N - 1, arglist>::type>) {
        auto [r, rest, extra_kwargs] =
            load_args_impl<typename Slice<0, N - 1, arglist>::type>(
                stack, nargs, nkwargs, spec);
        return std::tuple_cat(std::move(r), std::tuple<VT>{std::move(rest)});

      } else
        throw internal_error;
    } else if (spec.has_kwarg) {
      if constexpr (std::convertible_to<
                        KWT, typename GetNthType<N - 1, arglist>::type>) {
        auto [r, rest, extra_kwargs] =
            load_args_impl<typename Slice<0, N - 1, arglist>::type>(
                stack, nargs, nkwargs, spec);
        return std::tuple_cat(std::move(r),
                              std::tuple<KWT>{std::move(extra_kwargs)});

      } else
        throw internal_error;
    }
  }

  if (spec.has_vararg || spec.has_kwarg)
    throw slotmissing_error;

  auto [r, rest, extra_kwargs] =
      load_args_impl<arglist>(stack, nargs, nkwargs, spec);
  return r;
}

}  // namespace srbind
