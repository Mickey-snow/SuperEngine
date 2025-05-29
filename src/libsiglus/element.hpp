// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#pragma once

#include "libsiglus/element_code.hpp"
#include "libsiglus/value.hpp"

#include <memory>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace libsiglus {
namespace elm {
enum class Kind {
  Invalid,

  Usrcmd,
  Usrprop,

  Callprop,
  Memory,
  Farcall,
  GetTitle,
  SetTitle,
  Koe,
  Curcall,
  Selbtn
};
}

class IElement;
using Element = std::unique_ptr<IElement>;

class IElement {
 public:
  IElement(Type type_ = Type::Invalid);
  virtual ~IElement() = default;

  Type type;

  virtual elm::Kind Kind() const noexcept = 0;
  virtual std::string ToDebugString() const;
};

class UserCommand final : public IElement {
 public:
  std::string_view name;  // owned by Scene or Archive
  int scene, entry;

  elm::Kind Kind() const noexcept override;
  std::string ToDebugString() const override;
};

class UserProperty final : public IElement {
 public:
  std::string_view name;  // owned by Scene or Archive
  int scene, idx;

  elm::Kind Kind() const noexcept override;
  std::string ToDebugString() const override;
};

class UnknownElement final : public IElement {
 public:
  ElementCode elmcode;
  elm::Kind Kind() const noexcept override;
  std::string ToDebugString() const override;
};

// factory method
Element MakeElement(const ElementCode& elmcode);

}  // namespace libsiglus
