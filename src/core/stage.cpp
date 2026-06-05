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

#include <algorithm>
#include <limits>

Stage::Stage(int size)
    : foreground_objects(size),
      background_objects(size),
      next_objects(size),
      saved_foreground_objects(size),
      saved_background_objects(size) {}

void Stage::Reset() {
  foreground_objects.Clear();
  background_objects.Clear();
  next_objects.Clear();
}

void Stage::Wipe() {
  Wipe(std::numeric_limits<int>::min(), std::numeric_limits<int>::max(),
       std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
}

void Stage::Wipe(int begin_order,
                 int end_order,
                 int begin_layer,
                 int end_layer) {
  // TODO(siglus): This only handles object buffers. Promote mwnd, group,
  // btnsel, world, effect, and quake stage state when those core Siglus
  // element implementations exist.
  next_objects.Clear();

  const size_t count =
      std::min({foreground_objects.Size(), background_objects.Size(),
                next_objects.Size()});
  for (size_t i = 0; i < count; ++i) {
    const bool fg_exists = foreground_objects.Exists(i);
    const bool bg_exists = background_objects.Exists(i);

    auto in_range = [&](const ObjectParameter& param) {
      const int order = param.z_order, layer = param.z_layer;
      return (begin_order <= order && order <= end_order) &&
             (begin_layer <= layer && layer <= end_layer);
    };
    const bool front_in_range = in_range(foreground_objects[i].Param());
    if (!front_in_range && !bg_exists)
      continue;

    if (fg_exists)
      next_objects[i] = foreground_objects[i].Clone();

    const bool replace_front =
        bg_exists ||
        (fg_exists && foreground_objects[i].Param().wipe_copy == 0);
    if (!replace_front)
      continue;

    if (bg_exists) {
      foreground_objects[i] = std::move(background_objects[i]);
      background_objects.DeleteAt(i);
    } else {
      foreground_objects[i].FreeDataAndInitializeParams();
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

void Stage::TakeSavepointSnapshot() {
  auto& foreground = foreground_objects;
  auto& background = background_objects;

  saved_foreground_objects.Clear();
  for (auto it = foreground.begin(), end = foreground.end(); it != end; ++it) {
    saved_foreground_objects[it.pos()] = it->Clone();
  }

  saved_background_objects.Clear();
  for (auto it = background.begin(), end = background.end(); it != end; ++it) {
    saved_background_objects[it.pos()] = it->Clone();
  }

  saved_graphics_stack = graphics_stack;
}
