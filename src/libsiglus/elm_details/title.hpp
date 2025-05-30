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

namespace libsiglus::elm {

class SetTitle final : public IElement {
 public:
  // std::string title;

  constexpr elm::Kind Kind() const noexcept override { return Kind::SetTitle; }

  std::string ToDebugString() const override;
};

class GetTitle final : public IElement {
 public:
  // std::string title;

  constexpr elm::Kind Kind() const noexcept override { return Kind::GetTitle; }

  std::string ToDebugString() const override;
};

}  // namespace libsiglus::elm
