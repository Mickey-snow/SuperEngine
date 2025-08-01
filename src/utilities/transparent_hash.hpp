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

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

struct TransparentHash {
  using is_transparent = void;
  inline size_t operator()(std::string_view v) const noexcept {
    return std::hash<std::string_view>{}(v);
  }
  inline size_t operator()(std::string const& s) const noexcept {
    return std::hash<std::string_view>{}(s);
  }
};

struct TransparentEq {
  using is_transparent = void;
  inline bool operator()(std::string_view a,
                         std::string_view b) const noexcept {
    return a == b;
  }
};

template <typename T>
using transparent_hashmap =
    std::unordered_map<std::string, T, TransparentHash, TransparentEq>;

template <typename T>
using transparent_hashset =
    std::unordered_set<std::string, T, TransparentHash, TransparentEq>;
