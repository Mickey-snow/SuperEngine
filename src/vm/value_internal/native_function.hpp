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

#include "vm/value.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace serilang {

struct Fiber;

class NativeFunction : public IObject {
 public:
  static constexpr inline ObjType objtype = ObjType::Native;

  using function_t =
      std::function<Value(Fiber&,
                          std::vector<Value>,
                          std::unordered_map<std::string, Value>)>;

  NativeFunction(std::string name, function_t fn);

  std::string Name() const;
  constexpr ObjType Type() const noexcept final { return objtype; }

  std::string Str() const override;
  std::string Desc() const override;

  void Call(Fiber& f, uint8_t nargs, uint8_t nkwargs) final;

 private:
  std::string name_;
  function_t fn_;
};

}  // namespace serilang
