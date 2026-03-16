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

#include "vm/list.hpp"

#include "utilities/string_utilities.hpp"
#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <ranges>
#include <string>

namespace serilang {

std::string List::Str() const {
  return '[' +
         Join(",", std::views::all(items) |
                       std::views::transform(
                           [](const Value& v) { return v.Str(); })) +
         ']';
}
std::string List::Desc() const {
  return "<list[" + std::to_string(items.size()) + "]>";
}

void List::MarkRoots(GCVisitor& visitor) {
  for (auto& it : items)
    visitor.MarkSub(it);
}

void List::GetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.back());
  f.op_stack.pop_back();

  auto* index_ptr = idx.Get_if<int>();
  if (!index_ptr) {
    vm.Error(f, "list index must be integer, but got: " + idx.Desc());
    return;
  }

  int index = *index_ptr < 0 ? items.size() + *index_ptr : *index_ptr;
  if (index < 0 || index >= items.size()) {
    vm.Error(f, "list index '" + idx.Str() + "' out of range");
    return;
  }

  f.op_stack.back() = items[index];
}

void List::SetItem(VM& vm, Fiber& f) {
  Value idx = std::move(f.op_stack.end()[-2]),
        val = std::move(f.op_stack.end()[-1]);
  f.op_stack.resize(f.op_stack.size() - 3);  // (list, idx, val)

  auto* index_ptr = idx.Get_if<int>();
  if (!index_ptr) {
    vm.Error(f, "list index must be integer, but got: " + idx.Desc());
    return;
  }

  int index = *index_ptr < 0 ? items.size() + *index_ptr : *index_ptr;
  if (index < 0 || index >= items.size()) {
    vm.Error(f, "list index '" + idx.Str() + "' out of range");
    return;
  }

  items[index] = std::move(val);
}

}  // namespace serilang
