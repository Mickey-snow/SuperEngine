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

#include "libsiglus/argument_list.hpp"

#include "utilities/string_utilities.hpp"

#include <algorithm>
#include <format>

namespace libsiglus {

std::vector<std::string> ArgumentList::ToStringVec() const {
  std::vector<std::string> repr;
  repr.reserve(args.size());
  std::transform(args.cbegin(), args.cend(), std::back_inserter(repr),
                 [](const auto& x) {
                   return std::visit(
                       [](const auto& x) {
                         using T = std::decay_t<decltype(x)>;
                         if constexpr (std::same_as<T, Type>)
                           return ToString(x);
                         else
                           return '[' + x.ToDebugString() + ']';
                       },
                       x);
                 });
  return repr;
}
std::string ArgumentList::ToDebugString() const {
  return Join(",", ToStringVec());
}

std::string Signature::ToDebugString() const {
  std::vector<std::string> args_repr = arglist.ToStringVec();
  for (size_t i = arglist.size() - argtags.size(); i < arglist.size(); ++i) {
    args_repr[i] = std::format(
        "_{}={}", argtags[i - (arglist.size() - argtags.size())], args_repr[i]);
  }

  return std::format("[{}]({}) -> {}", overload_id,
                     Join(",", std::move(args_repr)), ToString(rettype));
}

Invoke::Invoke(int ol, std::vector<Value> arglist, Type rettype)
    : overload_id(ol), arg(std::move(arglist)), return_type(rettype) {}

}  // namespace libsiglus
