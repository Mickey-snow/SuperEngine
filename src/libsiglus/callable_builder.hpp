// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#pragma once

#include "libsiglus/element.hpp"
#include "libsiglus/types.hpp"

#include <array>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace libsiglus::elm {

// fn("foo")                       ->  name_builder
// fn("foo")[1]                    ->  overload_builder
// fn("foo")[1](Type::Int, ...)      ->  signature_builder
// fn("foo")[1](...).ret(Type::Int) ->  function_builder

struct function_builder;

template <class S>
concept string_like = requires(S s) { std::string_view(s); };

struct name_builder {
  std::string_view name;
  constexpr explicit name_builder(string_like auto&& n) : name(n) {}

  constexpr auto operator[](int index) const;  // -> overload_builder
};

template <size_t N>
struct signature_builder {
  std::string_view name;
  int index;
  std::array<Type, N> args;

  constexpr function_builder ret(Type r) const;
};

struct overload_builder {
  std::string_view name;
  int index;

  constexpr auto operator()() const {
    return signature_builder{name, index, std::array<Type, 0>{}};
  }
  template <typename... Ts>
  constexpr auto operator()(Ts... ts) const {
    return signature_builder{name, index, std::array{ts...}};
  }
};

struct function_builder {
  std::string_view name;
  int index;
  std::vector<Type> arg_vec;  // materialised once at runtime
  Type ret;

  constexpr Function build() const {
    return Function{name, std::move(arg_vec), ret};
  }
};

inline constexpr auto name_builder::operator[](int index) const {
  return overload_builder{name, index};
}
template <size_t N>
constexpr function_builder signature_builder<N>::ret(Type r) const {
  return function_builder{name, index,
                          std::vector<Type>(args.begin(), args.end()), r};
}

[[nodiscard]] constexpr name_builder fn(string_like auto&& s) {
  return name_builder{std::forward<decltype(s)>(s)};
}

template <class... FB>
[[nodiscard]] Callable make_callable(FB&&... fb) {
  Callable c;
  (c.overloads.emplace(fb.index, fb.build()), ...);
  return c;
}

}  // namespace libsiglus::elm
