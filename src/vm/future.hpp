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
#include "vm/iobject.hpp"
#include "vm/promise.hpp"
#include "vm/value.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace serilang {

struct Fiber;
struct GCVisitor;
class VM;

struct Future : public IObject {
  static constexpr inline ObjType objtype = ObjType::Future;

  std::shared_ptr<Promise> promise;

  Future();

  constexpr ObjType Type() const noexcept final { return objtype; }
  constexpr size_t Size() const noexcept final { return sizeof(*this); }

  inline void AddRoot(Value& v) { promise->AddRoot(v); }
  void MarkRoots(GCVisitor& visitor) override;

  std::string Str() const override;
  std::string Desc() const override;
};

namespace async_builtin {
Value Sleep(VM& vm, int ms, Value result);
Value Timeout(VM& vm, Value awaited, int ms);
Value Gather(VM& vm, std::vector<Value> awaitables);
Value Race(VM& vm, std::vector<Value> awaitables);
}  // namespace async_builtin

void InstallAsyncBuiltins(VM& m);

}  // namespace serilang
