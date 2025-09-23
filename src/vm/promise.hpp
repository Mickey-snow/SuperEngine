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

#include "utilities/expected.hpp"
#include "vm/value.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace serilang {

struct Fiber;
struct GCVisitor;
class IObject;

// Fibres are awaitable handles; Promises are hidden internal machinery
struct Promise {
  enum class Status { Pending, Resolved, Rejected };
  Status status = Status::Pending;

  // note: result.has_value() -> has result or exception
  // result->has_value() -> has result
  std::optional<expected<Value, std::string>> result = std::nullopt;
  std::vector<std::function<void(Promise*)>> wakers;

  std::vector<IObject*> roots;
  inline void AddRoot(Value& value) {
    if (auto* obj = value.Get_if<IObject>())
      roots.emplace_back(obj);
  }

  std::string ToDebugString() const;

  void Reset(Fiber* fiber);
  void WakeAll();
  void Resolve(Value value);
  void Reject(std::string msg);
};

}  // namespace serilang
