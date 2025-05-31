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
std::string Member::ToDebugString() const { return '.' + std::string(name); }
std::string Call::ToDebugString() const {
  return '(' + Join(",", vals_to_string(args)) + ')';
}
std::string Subscript::ToDebugString() const {
  return '[' + (idx.has_value() ? ToString(*idx) : std::string()) + ']';
}
std::string Val::ToDebugString() const { return ".<" + ToString(value) + '>'; }

// -----------------------------------------------------------------------
// class Builder
Builder::Builder(std::function<void(Ctx&)> a) : action_(std::move(a)) {}
Builder::Builder(Node product)
    : action_{[p = std::move(product)](Ctx& ctx) {
        ctx.chain.nodes.push_back(p);
        ctx.elmcode = ctx.elmcode.subspan(1);
      }} {}
void Builder::Build(Ctx& ctx) const { std::invoke(action_, ctx); }

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

AccessChain& AccessChain::Append(std::span<const Value> elmcode) {
  nodes.reserve(nodes.size() + elmcode.size());

  flat_map<Builder> const* mp;
  while ((mp = elm::GetMethodMap(GetType())) != nullptr) {
    if (!mp || elmcode.empty())
      break;
    if (!std::holds_alternative<Integer>(elmcode.front()))
      break;
    const int key = AsInt(elmcode.front());
    if (!mp->contains(key))
      break;

    Builder::Ctx ctx{.elmcode = elmcode, .chain = *this};
    mp->at(key).Build(ctx);
  }

  const auto cur_type = GetType();
  for (const auto& it : elmcode)
    nodes.emplace_back(elm::Node(cur_type, elm::Val(it)));

  return *this;
}

// -----------------------------------------------------------------------
inline static auto b(Node node) { return Builder(std::move(node)); }
flat_map<Builder> const* GetMethodMap(Type type) {
  switch (type) {
    case Type::IntList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(Type::Int, Subscript{ctx.elmcode[1]});
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
          id[3] | b({Type::IntList, Member("b1")}),
          id[4] | b({Type::IntList, Member("b2")}),
          id[5] | b({Type::IntList, Member("b4")}),
          id[7] | b({Type::IntList, Member("b8")}),
          id[6] | b({Type::IntList, Member("b16")}),
          id[10] | b({Type::Callable, Member("Init")}),
          id[2] | b({Type::Callable, Member("Resize")}),
          id[9] | b({Type::Callable, Member("Size")}),
          id[8] | b({Type::Callable, Member("Fill")}),
          id[1] | b({Type::Callable, Member("Set")}));
      return &mp;
    }

    case Type::StrList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(Type::String,
                                         Call("substr", {ctx.elmcode[1]}));
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
          id[3] | b({Type::Callable, Member("Init")}),
          id[2] | b({Type::Callable, Member("Resize")}),
          id[4] | b({Type::Callable, Member("Size")}));
      return &mp;
    }

    default:
      return nullptr;
  }
}
}  // namespace libsiglus::elm
