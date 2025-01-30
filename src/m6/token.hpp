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

#include "m6/expr_ast.hpp"

#include <string>
#include <variant>

namespace m6 {

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

struct Operator {
  Op op;
  auto operator<=>(const Operator& rhs) const = default;
};

struct Dollar {
  auto operator<=>(const Dollar& rhs) const = default;
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

}  // namespace tok

using Token = std::variant<tok::ID,
                           tok::WS,
                           tok::Int,
                           tok::Operator,
                           tok::Dollar,
                           tok::SquareL,
                           tok::SquareR,
                           tok::CurlyL,
                           tok::CurlyR,
                           tok::ParenthesisL,
                           tok::ParenthesisR>;

namespace tok {
struct DebugStringVisitor {
  std::string operator()(const tok::ID&) const;
  std::string operator()(const tok::WS&) const;
  std::string operator()(const tok::Int&) const;
  std::string operator()(const tok::Operator&) const;
  std::string operator()(const tok::Dollar&) const;
  std::string operator()(const tok::SquareL&) const;
  std::string operator()(const tok::SquareR&) const;
  std::string operator()(const tok::CurlyL&) const;
  std::string operator()(const tok::CurlyR&) const;
  std::string operator()(const tok::ParenthesisL&) const;
  std::string operator()(const tok::ParenthesisR&) const;
};
}  // namespace tok

}  // namespace m6
