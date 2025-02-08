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

#include "m6/value.hpp"
#include "utilities/mpl.hpp"

#include <map>
#include <optional>
#include <tuple>
#include <vector>

namespace m6 {

namespace internal {

template <typename T>
auto parse_impl(auto& begin, auto const& end) -> T {
  if constexpr (false)
    ;

  else if constexpr (std::same_as<T, int>) {
    Value it = *begin++;
    return std::any_cast<int>(it->Get());
  }

  else if constexpr (std::same_as<T, int*>) {
    Value it = *begin++;
    return static_cast<int*>(it->Getptr());
  }

  else if constexpr (std::same_as<T, std::optional<int>>) {
    if (begin == end)
      return std::nullopt;
    if ((*begin)->Type() != typeid(int))
      return std::nullopt;

    Value it = *begin++;
    return std::any_cast<int>(it->Get());
  }

  else if constexpr (std::same_as<T, std::vector<int>>) {
    std::vector<int> result;
    while (begin != end) {
      Value it = *begin++;
      result.emplace_back(std::any_cast<int>(it->Get()));
    }
    return result;
  }

  else if constexpr (std::same_as<T, std::string>) {
    Value it = *begin++;
    return std::any_cast<std::string>(it->Get());
  }

  else if constexpr (std::same_as<T, std::string*>) {
    Value it = *begin++;
    return static_cast<std::string*>(it->Getptr());
  }

  else if constexpr (std::same_as<T, std::optional<std::string>>) {
    if (begin == end)
      return std::nullopt;
    if ((*begin)->Type() != typeid(std::string))
      return std::nullopt;

    Value it = *begin++;
    return std::any_cast<std::string>(it->Get());
  }

  else if constexpr (std::same_as<T, std::vector<std::string>>) {
    std::vector<std::string> result;
    while (begin != end) {
      Value it = *begin++;
      result.emplace_back(std::any_cast<std::string>(it->Get()));
    }
    return result;
  }

  else {
    static_assert(false, "ParseArgs: no implementation found.");
  }
}

}  // namespace internal

template <typename... Ts>
auto ParseArgs(std::vector<Value> args) -> std::tuple<Ts...> {
  auto begin = args.begin(), end = args.end();
  return std::tuple{internal::parse_impl<Ts>(begin, end)...};
}

}  // namespace m6
