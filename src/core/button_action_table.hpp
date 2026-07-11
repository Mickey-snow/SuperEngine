// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "core/rect.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

class Gameexe;

enum class ButtonState { Normal, Hit, Push, Select, Disable };
std::optional<ButtonState> ParseState(std::string_view state);

class ButtonActionTable {
 public:
  struct State {
    int pattern = 0;
    Point rep_pos = Point(0, 0);
    int rep_tr = 255;
    int rep_bright = 0;
    int rep_dark = 0;

    bool operator==(const State&) const = default;
  };

  struct Entry {
    State normal;
    State hit;
    State push;
    State select;
    State disable;

    State GetState(ButtonState state) const;
    void SetState(ButtonState state, State value);

    bool operator==(const Entry&) const = default;
  };

  static ButtonActionTable ParseSiglus(Gameexe& gexe);
  static ButtonActionTable ParseReallive(Gameexe& gexe);

  inline std::size_t GetCount() const { return cnt_; }
  inline Entry GetEntry(std::size_t idx) const { return entry_.at(idx); }
  inline auto GetAllEntry() const { return entry_; }

 private:
  std::size_t cnt_ = 0;
  std::vector<Entry> entry_;
};
