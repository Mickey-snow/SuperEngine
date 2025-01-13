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
// Along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#pragma once

#include <string>
#include <variant>

namespace tok {

struct ID {
  std::string id;
  auto operator<=>(const ID& rhs) const = default;
};

struct WS {
  auto operator<=>(const WS& rhs) const = default;
};

struct Int {
  int value;
  auto operator<=>(const Int& rhs) const = default;
};

struct Dollar {
  auto operator<=>(const Dollar& rhs) const = default;
};

struct Plus {
  auto operator<=>(const Plus& rhs) const = default;
};

struct Minus {
  auto operator<=>(const Minus& rhs) const = default;
};

struct Mult {
  auto operator<=>(const Mult& rhs) const = default;
};

struct Div {
  auto operator<=>(const Div& rhs) const = default;
};

struct Eq {
  auto operator<=>(const Eq& rhs) const = default;
};

struct SquareL {
  auto operator<=>(const SquareL& rhs) const = default;
};

struct SquareR {
  auto operator<=>(const SquareR& rhs) const = default;
};

struct CurlyL {
  auto operator<=>(const CurlyL& rhs) const = default;
};

struct CurlyR {
  auto operator<=>(const CurlyR& rhs) const = default;
};

struct ParenthesisL {
  auto operator<=>(const ParenthesisL& rhs) const = default;
};

struct ParenthesisR {
  auto operator<=>(const ParenthesisR& rhs) const = default;
};

struct AngleL {
  auto operator<=>(const AngleL& rhs) const = default;
};

struct AngleR {
  auto operator<=>(const AngleR& rhs) const = default;
};

struct Comma {
  auto operator<=>(const Comma& rhs) const = default;
};

struct Exclam {
  auto operator<=>(const Exclam& rhs) const = default;
};

}  // namespace tok

using Token = std::variant<tok::ID,
                           tok::WS,
                           tok::Int,
                           tok::Dollar,
                           tok::Plus,
                           tok::Minus,
                           tok::Mult,
                           tok::Div,
                           tok::Eq,
                           tok::SquareL,
                           tok::SquareR,
                           tok::CurlyL,
                           tok::CurlyR,
                           tok::ParenthesisL,
                           tok::ParenthesisR,
                           tok::AngleL,
                           tok::AngleR,
                           tok::Comma,
                           tok::Exclam>;
