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

#include "vm/dict.hpp"

#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

namespace serilang {

Dict::Dict(map_t m) : map(std::move(m)) {}

std::string Dict::Str() const {
  std::string repr;
  std::vector<const map_t::value_type*> items;
  items.reserve(map.size());
  for (const auto& item : map)
    items.push_back(&item);

  std::ranges::sort(
      items, [](const map_t::value_type* lhs, const map_t::value_type* rhs) {
        return lhs->first.Desc() < rhs->first.Desc();
      });

  for (const auto* item : items) {
    const auto& [k, v] = *item;
    if (!repr.empty())
      repr += ',';
    repr += k.Str() + ':' + v.Str();
  }

  return '{' + repr + '}';
}
std::string Dict::Desc() const {
  return "<dict{" + std::to_string(map.size()) + "}>";
}

void Dict::MarkRoots(GCVisitor& visitor) {
  for (auto& [k, it] : map) {
    visitor.MarkSub(const_cast<Value&>(k));
    visitor.MarkSub(it);
  }
}

void Dict::GetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.back());
  f.op_stack.pop_back();

  auto it = map.find(idx);
  if (it == map.cend()) {
    vm.Error(f, "dictionary has no key: " + idx.Str());
    return;
  }

  f.op_stack.back() = it->second;
}

void Dict::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.end()[-2]),
        val = std::move(f.op_stack.end()[-1]);
  f.op_stack.resize(f.op_stack.size() - 3);  // (dict, idx, val)

  map[idx] = std::move(val);
}

}  // namespace serilang
