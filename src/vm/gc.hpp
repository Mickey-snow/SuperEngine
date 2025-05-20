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

#pragma once

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

namespace serilang {

class Value;
class IObject;
class VM;
template <typename T>
concept is_object = std::derived_from<T, IObject>;

struct GCHeader {
  IObject* next = nullptr;
  bool marked = false;
  size_t size = 0;
};

class GarbageCollector {
 public:
  GarbageCollector() = default;
  ~GarbageCollector();

  template <typename T, typename... Args>
  auto Allocate(Args&&... args) -> std::add_pointer_t<T>
    requires is_object<T>
  {
    auto obj = static_cast<T*>(::operator new(sizeof(T)));
    allocated_bytes_ += sizeof(T);

    new (obj) T(std::forward<Args>(args)...);
    obj->hdr_.next = gc_list_;
    obj->hdr_.size = sizeof(T);
    gc_list_ = obj;

    return obj;
  }

  size_t AllocatedBytes() const;

  void UnmarkAll() const;
  void MarkRoot(VM& vm);
  void MarkRoot(Value& v);
  void MarkRoot(IObject* obj);
  void Sweep();

 private:
  IObject* gc_list_ = nullptr;
  size_t allocated_bytes_ = 0;
};

}  // namespace serilang
