// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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
// -----------------------------------------------------------------------

#include "core/stage.hpp"

Stage::Stage(int size)
    : foreground_objects(size),
      background_objects(size),
      saved_foreground_objects(size),
      saved_background_objects(size) {}

void Stage::Reset() {
  foreground_objects.Clear();
  background_objects.Clear();
}

void Stage::Wipe() {
  auto bg = background_objects.fbegin();
  const auto bg_end = background_objects.fend();
  auto fg = foreground_objects.fbegin();
  const auto fg_end = foreground_objects.fend();
  for (; bg != bg_end && fg != fg_end; bg++, fg++) {
    if (fg.valid() && !fg->Param().wipe_copy) {
      fg->InitializeParams();
      fg->FreeObjectData();
    }

    if (bg.valid()) {
      *fg = std::move(*bg);
    }
  }
}

void Stage::AddGraphicsStackCommand(std::string command) {
  graphics_stack.emplace_back(std::move(command));

  // RealLive only allows 127 commands to be on the stack so game programmers
  // can be lazy and not clear it.
  if (graphics_stack.size() > 127)
    graphics_stack.pop_front();
}
