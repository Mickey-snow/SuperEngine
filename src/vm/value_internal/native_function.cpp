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

#include "vm/value_internal/native_function.hpp"

#include "vm/value_internal/fiber.hpp"

#include <functional>
#include <utility>

namespace serilang {

NativeFunction::NativeFunction(std::string name, function_t fn)
    : name_(std::move(name)), fn_(std::move(fn)) {}

std::string NativeFunction::Name() const { return name_; }

std::string NativeFunction::Str() const { return "<fn " + name_ + '>'; }

std::string NativeFunction::Desc() const {
  return "<native function '" + name_ + "'>";
}

void NativeFunction::Call(Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::vector<Value> args;
  std::unordered_map<std::string, Value> kwargs;

  args.reserve(nargs);
  for (size_t i = f.stack.size() - nargs; i < f.stack.size(); ++i)
    args.emplace_back(std::move(f.stack[i]));

  auto retval = std::invoke(fn_, f, std::move(args), std::move(kwargs));

  f.stack.resize(f.stack.size() - nargs);
  f.stack.back() = std::move(retval);
}

}  // namespace serilang
