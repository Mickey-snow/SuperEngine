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

#include "m6/ast.hpp"
#include "m6/token.hpp"
#include "utilities/string_utilities.hpp"

#include <string_view>
#include <vector>

template <typename... Ts>
std::vector<m6::Token> TokenArray(Ts&&... args) {
  std::vector<m6::Token> result;
  result.reserve(sizeof...(args));
  (result.emplace_back(std::forward<Ts>(args)), ...);
  return result;
}

auto TokenArray(std::string_view sv) -> std::vector<m6::Token>;

#define EXPECT_TXTEQ(lhs, rhs) EXPECT_EQ(trim_cp(lhs), trim_cp(rhs));
