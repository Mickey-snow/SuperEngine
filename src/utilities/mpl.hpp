// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include <cstddef>

// Compile time constants
template <typename>
inline constexpr bool always_false = false;

// TypeList
struct NullType {};

template <typename... Ts>
struct TypeList {};

template <typename T, typename List>
struct AddFront;

template <typename T, typename... Ts>
struct AddFront<T, TypeList<Ts...>> {
  using type = TypeList<T, Ts...>;
};

template <typename List, typename T>
struct AddBack;

template <typename... Ts, typename T>
struct AddBack<TypeList<Ts...>, T> {
  using type = TypeList<Ts..., T>;
};

template <std::size_t N, typename List>
struct GetNthType;

template <std::size_t N, typename T, typename... Ts>
struct GetNthType<N, TypeList<T, Ts...>> {
  static_assert(N < sizeof...(Ts) + 1, "Index out of bounds");
  using type = typename GetNthType<N - 1, TypeList<Ts...>>::type;
};

template <typename T, typename... Ts>
struct GetNthType<0, TypeList<T, Ts...>> {
  using type = T;
};

template <typename List>
struct SizeOfTypeList;

template <typename... Ts>
struct SizeOfTypeList<TypeList<Ts...>> {
  static constexpr std::size_t value = sizeof...(Ts);
};

template <typename List1, typename List2>
struct Append;

template <typename... Ts, typename... Us>
struct Append<TypeList<Ts...>, TypeList<Us...>> {
  using type = TypeList<Ts..., Us...>;
};

template <template <typename...> class Dest, typename List>
struct Unpack;

template <template <typename...> class Dest, typename... Ts>
struct Unpack<Dest, TypeList<Ts...>> {
  using type = Dest<Ts...>;
};
