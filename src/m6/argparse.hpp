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

#include "m6/exception.hpp"
#include "utilities/mpl.hpp"
#include "vm/value.hpp"

#include <map>
#include <optional>
#include <tuple>
#include <vector>

namespace m6 {

namespace internal {

using serilang::ObjType;
using serilang::Value;

template <typename T>
concept is_optional = requires { typename T::value_type; } &&
                      std::same_as<T, std::optional<typename T::value_type>>;

template <typename T>
concept is_vector = requires { typename T::value_type; } &&
                    std::same_as<T, std::vector<typename T::value_type>>;

template <typename T>
concept is_sharedptr = requires {
  typename T::element_type;
} && std::same_as<T, std::shared_ptr<typename T::element_type>>;

template <typename T>
concept is_parser = requires(T x, Value& val) {
  { x.Parsable(const_cast<const Value&>(val)) } -> std::convertible_to<bool>;
  { x.Parse(val) };
} && std::is_constructible_v<T>;

class IntParser {
 public:
  IntParser() = default;
  bool Parsable(const Value& val) noexcept {
    return val.Type() == ObjType::Int;
  }
  void Parse(Value& val) { value = val.Get_if<int>(); }
  int* value;
};
static_assert(is_parser<IntParser>);

class StrParser {
 public:
  StrParser() = default;
  bool Parsable(const Value& val) noexcept {
    return val.Type() == ObjType::Str;
  }
  void Parse(Value& val) { value = val.Get_if<std::string>(); }
  std::string* value;
};
static_assert(is_parser<StrParser>);

template <typename T>
auto CreateParser() {
  if constexpr (false)
    ;
  else if constexpr (std::same_as<T, int>)
    return IntParser();
  else if constexpr (std::same_as<T, std::string>)
    return StrParser();
  else
    static_assert(always_false<T>);
}

template <typename T>
auto parse_impl(auto& begin, auto const& end) -> T {
  if constexpr (false)
    ;

  else if constexpr (std::is_pointer_v<T>) {  // reference
    auto parser = CreateParser<std::remove_pointer_t<T>>();
    if (begin == end)
      throw SyntaxError("Not enough arguments provided.");

    Value& it = *begin;
    if (!parser.Parsable(it))
      throw TypeError("No viable convertion from " + it.Desc());

    ++begin;
    parser.Parse(it);
    return parser.value;
  }

  else if constexpr (is_optional<T>) {  // optional value
    try {
      return parse_impl<typename T::value_type>(begin, end);
    } catch (...) {
      return std::nullopt;
    }
  }

  else if constexpr (is_vector<T>) {  // argument list
    T result;
    while (begin != end)
      result.emplace_back(parse_impl<typename T::value_type>(begin, end));
    return result;
  }

  else {
    auto parser = CreateParser<T>();
    if (begin == end)
      throw SyntaxError("Not enough arguments provided.");

    Value& it = *begin;
    if (!parser.Parsable(it))
      throw TypeError("No viable convertion from " + it.Desc());

    ++begin;
    parser.Parse(it);
    return *parser.value;
  }
}

}  // namespace internal

template <typename... Ts>
auto ParseArgs(auto begin, const auto end) -> std::tuple<Ts...> {
  auto result = std::tuple{internal::parse_impl<Ts>(begin, end)...};
  if (begin != end)
    throw SyntaxError("Too many arguments provided.");
  return result;
}

template <typename... Ts>
auto ParseArgs(TypeList<Ts...> /*unused*/, auto begin, const auto end)
    -> std::tuple<Ts...> {
  return ParseArgs<Ts...>(begin, end);
}

}  // namespace m6
