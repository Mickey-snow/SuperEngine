// -----------------------------------------------------------------------
//
// This file is part of RLVM
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

#ifndef SRC_LIBSIGLUS_VALUE_HPP_
#define SRC_LIBSIGLUS_VALUE_HPP_

#include "types.hpp"

#include <string>
#include <variant>

namespace libsiglus {

class Integer {
 public:
  Integer(int value = 0) : val_(value) {}

  operator int() const { return val_; }

  std::string ToDebugString() const { return "int:" + std::to_string(val_); }

  int val_;
};

class String {
 public:
  String(std::string value = "") : val_(std::move(value)) {}

  operator std::string() const { return val_; }

  std::string ToDebugString() const { return "str:" + val_; }

  std::string val_;
};

// represents a siglus property
using Value = std::variant<Integer, String>;

struct DebugStringOf {
  std::string operator()(std::monostate) { return ""; }

  template <typename T>
  auto operator()(const T& it) {
    return it.ToDebugString();
  }
};

}  // namespace libsiglus

#endif
