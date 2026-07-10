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

#include "core/object_internal/animator.hpp"
#include "core/object_internal/objdrawer.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

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
  ClearTransitionRenderState();
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

  auto InWipeRange = [begin = std::make_pair(begin_order, begin_layer),
                      end = std::make_pair(end_order, end_layer)](
                         std::pair<int, int> now) -> bool {
    return begin <= now && now <= end;
  };
  auto ObjOrd = [](const ObjectParameter& param) {
    return std::make_pair(param.z_order, param.z_layer);
  };

  const size_t count =
      std::min({foreground_objects.Size(), background_objects.Size(),
                next_objects.Size()});
  for (size_t i = 0; i < count; ++i) {
    const bool fg_exists = foreground_objects.Exists(i);
    const bool bg_exists = background_objects.Exists(i);

    const bool front_in_range =
        fg_exists &&
        InWipeRange(ObjOrd(foreground_objects.At(i).value().Param()));
    const bool back_participates =
        bg_exists && (background_objects[i].HasDrawer() ||
                      background_objects[i].HasChildren() ||
                      background_objects[i].Param().wipe_erase != 0);
    if (!front_in_range && !back_participates)
      continue;

    if (fg_exists)
      next_objects[i] = foreground_objects[i].Clone();

    const bool replace_front =
        back_participates || (front_in_range && fg_exists &&
                              foreground_objects[i].Param().wipe_copy == 0);
    if (!replace_front)
      continue;

    if (back_participates) {
      foreground_objects[i] = std::move(background_objects[i]);
      background_objects.DeleteAt(i);
    } else {
      foreground_objects[i].FreeDataAndInitializeParams();
    }
  }
}

void Stage::SetTransitionRenderAlpha(double foreground_alpha,
                                     double next_alpha) {
  foreground_render_alpha_ = std::clamp(foreground_alpha, 0.0, 1.0);
  next_render_alpha_ = std::clamp(next_alpha, 0.0, 1.0);
}

void Stage::ClearTransitionRenderState() {
  foreground_render_alpha_ = 1.0;
  next_render_alpha_ = 0.0;
}

LazyArray<GraphicsObject>& Stage::ObjectsForLayer(int layer) {
  switch (layer) {
    case kLayerFg:
      return foreground_objects;
    case kLayerBg:
      return background_objects;
    case kLayerNext:
      return next_objects;
    default:
      throw std::runtime_error("Invalid layer number");
  }
}

const LazyArray<GraphicsObject>& Stage::ObjectsForLayer(int layer) const {
  switch (layer) {
    case kLayerFg:
      return foreground_objects;
    case kLayerBg:
      return background_objects;
    case kLayerNext:
      return next_objects;
    default:
      throw std::runtime_error("Invalid layer number");
  }
}

GraphicsObject& Stage::GetObject(int layer, int obj_number) {
  return ObjectsForLayer(layer)[obj_number];
}

size_t Stage::GetFreeObjectId(int layer) {
  LazyArray<GraphicsObject>& objects = ObjectsForLayer(layer);

  for (size_t i = 0, end = objects.Size(); i < end; ++i) {
    if (!objects.Exists(i))
      return i;
  }

  throw std::runtime_error("No free object slots");
}

void Stage::SetObject(int layer, int obj_number, GraphicsObject&& object) {
  ObjectsForLayer(layer)[obj_number] = std::move(object);
}

void Stage::RemoveObject(int layer, size_t obj_number) {
  ObjectsForLayer(layer).DeleteAt(obj_number);
}

void Stage::FreeObjectData(int obj_number) {
  foreground_objects[obj_number].FreeObjectData();
  background_objects[obj_number].FreeObjectData();
}

void Stage::FreeAllObjectData() {
  FreeLayerObjectData(kLayerFg);
  FreeLayerObjectData(kLayerBg);
  FreeLayerObjectData(kLayerNext);
}

void Stage::FreeLayerObjectData(int layer) {
  LazyArray<GraphicsObject>& objects = ObjectsForLayer(layer);
  for (auto& obj : objects)
    obj.FreeObjectData();
}

void Stage::InitializeObjectParams(int obj_number) {
  foreground_objects[obj_number].InitializeParams();
  background_objects[obj_number].InitializeParams();
}

void Stage::InitializeAllObjectParams() {
  for (GraphicsObject& object : foreground_objects)
    object.InitializeParams();

  for (GraphicsObject& object : background_objects)
    object.InitializeParams();
}

void Stage::Execute() {
  for (auto& obj : foreground_objects) {
    obj.Execute();
    obj.ExecuteMutators();
  }
  for (auto& obj : background_objects) {
    obj.Execute();
    obj.ExecuteMutators();
  }
  for (auto& obj : next_objects) {
    obj.Execute();
    obj.ExecuteMutators();
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
