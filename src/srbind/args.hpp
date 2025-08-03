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

#include "srbind/caster.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace srbind {

// -------------------------------------------------------------
// named argument descriptor
// -------------------------------------------------------------
struct arg_t {
  std::string name;
  bool kw_only = false;
  bool has_default = false;
  std::function<serilang::TempValue()> make_default;

  explicit arg_t(const char* n) : name(n) {}

  // allow: arg("x") = 42  (captures by value)
  template <class T>
  arg_t operator=(T&& v) && {
    arg_t r = *this;
    r.has_default = true;
    auto held = std::make_shared<std::decay_t<T>>(std::forward<T>(v));
    r.make_default = [held]() -> serilang::TempValue {
      return type_caster<std::decay_t<T>>::cast(*held);
    };
    return r;
  }
};

inline arg_t arg(const char* name) { return arg_t{name}; }
inline arg_t kw_arg(const char* name) {
  auto arg = arg_t{name};
  arg.kw_only = true;
  return arg;
}

struct vararg_t {};
inline constexpr vararg_t vararg;

struct kwargs_t {};
inline constexpr kwargs_t kwargs;

using argument = std::variant<arg_t, vararg_t, kwargs_t>;

template <class T>
concept argument_like = std::same_as<std::remove_cvref_t<T>, arg_t> ||
                        std::same_as<std::remove_cvref_t<T>, vararg_t> ||
                        std::same_as<std::remove_cvref_t<T>, kwargs_t>;

}  // namespace srbind
