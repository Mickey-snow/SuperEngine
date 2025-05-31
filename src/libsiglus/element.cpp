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

namespace libsiglus ::elm {

std::string Usrcmd::ToDebugString() const {
  return std::format("@{}.{}:{}", scene, entry, name);
}
std::string Usrprop::ToDebugString() const {
  return std::format("@{}.{}:{}", scene, idx, name);
}
std::string Mem::ToDebugString() const { return std::string{bank}; }
std::string Sym::ToDebugString() const {
  std::string repr = name;
  if (kidoku.has_value())
    repr += '#' + std::to_string(*kidoku);
  return repr;
}
std::string Arg::ToDebugString() const { return "arg_" + std::to_string(id); }
std::string Member::ToDebugString() const { return std::string(name); }
std::string Subscript::ToDebugString() const {
  return std::format("[{}]", idx.has_value() ? ToString(*idx) : std::string());
}
std::string Val::ToDebugString() const { return ToString(value); }
std::string AccessChain::ToDebugString() const {
  std::string repr = root.ToDebugString();
  for (const auto& it : nodes)
    repr += ',' + it.ToDebugString();
  return repr;
}

Type AccessChain::GetType() const {
  if (nodes.empty())
    return root.type;
  else
    return nodes.back().type;
}

AccessChain& AccessChain::Append(std::span<const Value> elmcode) {
  nodes.reserve(nodes.size() + elmcode.size());

  Type cur_type = GetType();
  for (auto* mp = elm::GetMethodMap(cur_type); mp && !elmcode.empty();) {
    if (!std::holds_alternative<Integer>(elmcode.front()))
      break;
    elm::Node next = mp->at(AsInt(elmcode.front()));

    cur_type = next.type;
    elmcode = elmcode.subspan(1);

    mp = elm::GetMethodMap(cur_type);
    nodes.emplace_back(std::move(next));
  }

  for (const auto& it : elmcode)
    nodes.emplace_back(elm::Node(cur_type, elm::Val(it)));

  return *this;
}

// -----------------------------------------------------------------------
flat_map<Node> const* GetMethodMap(Type type) {
  switch (type) {
    case Type::IntList: {
      static const auto mp =
          make_flatmap<Node>(id[-1] | Node{Type::Int, Subscript()},
                             id[3] | Node{Type::IntList, Member("b1")},
                             id[4] | Node{Type::IntList, Member("b2")},
                             id[5] | Node{Type::IntList, Member("b4")},
                             id[7] | Node{Type::IntList, Member("b8")},
                             id[6] | Node{Type::IntList, Member("b16")},
                             id[10] | Node{Type::None, Member("Init")},
                             id[2] | Node{Type::None, Member("Resize")},
                             id[9] | Node{Type::Int, Member("Size")},
                             id[8] | Node{Type::Int, Member("Fill")},
                             id[1] | Node{Type::Int, Member("Set")});
      return &mp;
    }

    default:
      return nullptr;
  }
}
}  // namespace libsiglus::elm
