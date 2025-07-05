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

#include "libsiglus/function.hpp"
#include "libsiglus/value.hpp"

#include <algorithm>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace libsiglus::elm {

class ElementCode {
 public:
  std::vector<Value> code;

  // bind context
  bool force_bind;
  Invoke bind_ctx;

  ElementCode();
  ElementCode(std::initializer_list<int> it) : ElementCode() {
    code.reserve(it.size());
    std::transform(it.begin(), it.end(), std::back_inserter(code),
                   [](int x) { return Value(Integer(x)); });
  }
  template <typename T, typename U>
  ElementCode(T&& begin, U&& end)
      : code(std::forward<T>(begin), std::forward<U>(end)),
        force_bind(false),
        bind_ctx() {}

  void ForceBind(Invoke ctx);

  Value& operator[](size_t idx) { return code[idx]; }
  template <typename T>
  auto At(size_t idx) const {
    if constexpr (std::same_as<T, Value>)
      return code[idx];
    else if constexpr (std::same_as<T, int>) {
      if (!std::holds_alternative<Integer>(code[idx]))
        throw std::runtime_error("ElementCode: Expected integer, but got " +
                                 ToString(code[idx]));
      return AsInt(code[idx]);
    } else if constexpr (std::same_as<T, std::string>) {
      if (!std::holds_alternative<String>(code[idx]))
        throw std::runtime_error("ElementCode: Expected string, but got " +
                                 ToString(code[idx]));
      return AsStr(code[idx]);
    }
  }

  auto IntegerView() const {
    return std::views::all(code) | std::views::transform([](const Value& v) {
             if (!std::holds_alternative<Integer>(v))
               throw std::runtime_error(
                   "ElementCode: Expected integer, but got " + ToString(v));
             return AsInt(v);
           });
  }

  // for testing
  bool operator==(const ElementCode& rhs) const;
  friend std::ostream& operator<<(std::ostream& os, const ElementCode& elm) {
    os << "elm<";
    bool first = true;
    for (auto const& it : elm.code) {
      if (!first)
        os << ',';
      first = false;

      os << ToString(it);
    }
    os << '>';
    return os;
  }
};

}  // namespace libsiglus::elm
