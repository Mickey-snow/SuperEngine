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

#include "libsiglus/element.hpp"

#include "utilities/string_utilities.hpp"

#include <format>
#include <numeric>

namespace libsiglus::elm {

std::string Usrcmd::ToDebugString() const {
  return std::format("@{}.{}:{}", scene, entry, name);
}
std::string Usrprop::ToDebugString() const {
  return std::format("@{}.{}:{}", scene, idx, name);
}
std::string Mem::ToDebugString() const { return std::string{bank}; }
std::string Sym::ToDebugString() const { return name; }
std::string Print::ToDebugString() const { return "print"; }
std::string Arg::ToDebugString() const { return "arg_" + std::to_string(id); }
std::string Member::ToDebugString() const { return '.' + std::string(name); }
std::string Call::ToDebugString() const {
  return std::format(".{}({})", name, Join(",", vals_to_string(args)));
}
std::string Subscript::ToDebugString() const {
  return '[' + (idx.has_value() ? ToString(*idx) : std::string()) + ']';
}
std::string Val::ToDebugString() const { return ".<" + ToString(value) + '>'; }

// -----------------------------------------------------------------------
// AccessChain
std::string AccessChain::ToDebugString() const {
  return std::accumulate(
      nodes.cbegin(), nodes.cend(), root.ToDebugString(),
      [](std::string&& s, auto const& v) { return s + v.ToDebugString(); });
}

Type AccessChain::GetType() const {
  if (nodes.empty())
    return root.type;
  else
    return nodes.back().type;
}

}  // namespace libsiglus::elm
