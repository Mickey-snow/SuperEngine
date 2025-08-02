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
#include <functional>
#include <type_traits>

// -----------------------------------------------------------------------
// Compile time constants
// -----------------------------------------------------------------------
template <typename>
inline constexpr bool always_false = false;

// -----------------------------------------------------------------------
// TypeList
// -----------------------------------------------------------------------
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

template <typename T, typename List>
struct Contains;

template <typename T, typename... Ts>
struct Contains<T, TypeList<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <std::size_t Begin, std::size_t Count, typename List>
struct Slice;
namespace detail {
// Offset an index_sequence by a constant Begin.
template <std::size_t Begin, typename Seq>
struct offset_sequence;
template <std::size_t Begin, std::size_t... Is>
struct offset_sequence<Begin, std::index_sequence<Is...>> {
  using type = std::index_sequence<(Begin + Is)...>;
};
// Gather types at given indices into a TypeList.
template <typename List, typename Seq>
struct gather;
template <typename List, std::size_t... Is>
struct gather<List, std::index_sequence<Is...>> {
  using type = TypeList<typename GetNthType<Is, List>::type...>;
};
}  // namespace detail

template <std::size_t Begin, std::size_t Count, typename... Ts>
struct Slice<Begin, Count, TypeList<Ts...>> {
  static_assert(Begin <= sizeof...(Ts), "Begin out of bounds");
  static_assert(Begin + Count <= sizeof...(Ts), "Slice exceeds list size");

  using indices_rel = std::make_index_sequence<Count>;
  using indices_abs =
      typename detail::offset_sequence<Begin, indices_rel>::type;

  using type = typename detail::gather<TypeList<Ts...>, indices_abs>::type;
};

// -----------------------------------------------------------------------
// function_traits
// -----------------------------------------------------------------------
template <typename T>
struct function_traits;

// Primary specialization: plain function types.
template <typename R, typename... Args>
struct function_traits<R(Args...)> {
  using result_type = R;
  using argument_types = TypeList<Args...>;
};

// Specialization for noexcept functions.
template <typename R, typename... Args>
struct function_traits<R(Args...) noexcept> : function_traits<R(Args...)> {};

// Function pointer.
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {};

// Function pointer with noexcept.
template <typename R, typename... Args>
struct function_traits<R (*)(Args...) noexcept>
    : function_traits<R(Args...) noexcept> {};

// Member function pointers (non-const).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {};

// Member function pointers (non-const) noexcept.
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) noexcept>
    : function_traits<R(Args...) noexcept> {};

// Member function pointers (const).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

// Member function pointers (const) noexcept.
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const noexcept>
    : function_traits<R(Args...) noexcept> {};

// Member function pointers (volatile).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile>
    : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile noexcept>
    : function_traits<R(Args...) noexcept> {};

// Member function pointers (const volatile).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const volatile>
    : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const volatile noexcept>
    : function_traits<R(Args...) noexcept> {};

// Ref-qualified member functions (lvalue ref).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) &> : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const&> : function_traits<R(Args...)> {
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile&>
    : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const volatile&>
    : function_traits<R(Args...)> {};

// Ref-qualified member functions (rvalue ref).
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) &&> : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const&&>
    : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile&&>
    : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const volatile&&>
    : function_traits<R(Args...)> {};

// std::function specialization
template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>>
    : function_traits<R(Args...)> {};

template <typename F>
struct function_traits
    : function_traits<decltype(&std::remove_reference_t<F>::operator())> {};
