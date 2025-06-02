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

#include "utilities/string_utilities.hpp"

#include <algorithm>
#include <format>

namespace libsiglus {
namespace lex {

// helper to count the length of bytes of a `ArgumentList`
static size_t count_arglist(const ArgumentList& al) {
  size_t len = 4 + al.size() * 4;
  for (const auto& it : al.args)
    if (std::holds_alternative<ArgumentList>(it))
      len += count_arglist(std::get<ArgumentList>(it));
  return len;
}
size_t Command::ByteLength() const {
  size_t len = 13;
  len += sig.argtags.size() * 4;
  len += count_arglist(sig.arglist);
  return len;
}

std::string Command::ToDebugString() const {
  return "cmd" + sig.ToDebugString();
}

std::string Gosub::ToDebugString() const {
  return std::format("gosub@{} ({}) -> {}", label_, argt_.ToDebugString(),
                     ToString(return_type_));
}
size_t Gosub::ByteLength() const { return 5 + count_arglist(argt_); }

std::string Return::ToDebugString() const {
  return std::format("ret({})", ret_types_.ToDebugString());
}
size_t Return::ByteLength() const { return 1 + count_arglist(ret_types_); }

std::string Declare::ToDebugString() const {
  return std::format("declare {} {}", ToString(type), size);
}
}  // namespace lex
}  // namespace libsiglus
