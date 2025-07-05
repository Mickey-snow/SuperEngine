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

#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"

#include <variant>
#include <vector>

namespace libsiglus {

struct ArgumentList {
  using node_t = std::variant<Type, ArgumentList>;
  std::vector<node_t> args;

  inline auto size() const { return args.size(); }
  std::string ToDebugString() const;
  std::vector<std::string> ToStringVec() const;
};

struct Signature {
  int overload_id;
  ArgumentList arglist;
  std::vector<int> argtags;
  Type rettype;

  std::string ToDebugString() const;
};

struct Invoke {
  int overload_id = 0;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type = Type::None;

  Invoke() = default;
  Invoke(int ol, std::vector<Value> arg, Type ret = Type::None);

  std::string ToDebugString() const;
};

}  // namespace libsiglus
