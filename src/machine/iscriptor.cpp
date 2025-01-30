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

#include "machine/iscriptor.hpp"

#include <string>

ScriptLocation::ScriptLocation()
    : scenario_number(0), location_offset(0), line_num(-1) {}

ScriptLocation::ScriptLocation(int scenario_id, std::size_t off)
    : scenario_number(scenario_id), location_offset(off), line_num(-1) {}

std::string ScriptLocation::DebugString() const {
  return '(' + std::to_string(scenario_number) + ':' +
         (line_num > 0 ? std::to_string(line_num) : "???") + ')';
}
