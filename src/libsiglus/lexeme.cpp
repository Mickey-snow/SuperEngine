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

#include "libsiglus/lexeme.hpp"

#include "libsiglus/strutil.hpp"

#include <algorithm>
#include <format>

namespace libsiglus {
namespace lex {

std::string Command::ToDebugString() const {
  std::vector<std::string> args_repr;
  args_repr.reserve(arg_.size());

  std::transform(arg_.cbegin(), arg_.cend(), std::back_inserter(args_repr),
                 [](const auto& x) { return ToString(x); });
  for (size_t i = arg_.size() - arg_tag_.size(); i < arg_.size(); ++i) {
    args_repr[i] = std::format(
        "_{}={}", arg_tag_[i - (arg_.size() - arg_tag_.size())], args_repr[i]);
  }

  return std::format("cmd[{}]({}) -> {}", override_,
                     Join(",", std::move(args_repr)), ToString(rettype_));
}

std::string Gosub::ToDebugString() const {
  return std::format(
      "gosub@{} ({}) -> {}",
      label_,  // NOLINT
      Join(",", argt_ | std::views::transform(
                            [](const auto& it) { return ToString(it); })),
      ToString(return_type_));
}

std::string Return::ToDebugString() const {
  return std::format(
      "ret({})",
      Join(",", ret_types_ | std::views::transform(
                                 [](const auto& it) { return ToString(it); })));
}

}  // namespace lex
}  // namespace libsiglus
