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

#include "libsiglus/element.hpp"

#include <variant>

namespace libsiglus::elm {

class Memory final : public IElement {
 public:
  enum class Bank {
    A = 25,
    B = 26,
    C = 27,
    D = 28,
    E = 29,
    F = 30,
    X = 137,
    G = 31,
    Z = 32,
    S = 34,
    M = 35,
    H,
    I,
    J,
    L,
    K,
    local_name = 106,
    global_name = 107
  };

  struct Access {
    int idx = -1;
  };
  struct Init {};
  struct Resize {};
  struct Fill {};
  struct Size {};
  struct Set {};

  Bank bank;
  int bits = 32;
  std::variant<std::monostate, Access, Init, Resize, Fill, Size, Set> var;

  Element Parse(std::span<int> path) const override;

  elm::Kind Kind() const noexcept override;
  std::string ToDebugString() const override;
};

}  // namespace libsiglus::elm
