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
                         const ObjectParameter& param) -> bool {
    const auto now = std::make_pair(param.z_order, param.z_layer);
    return begin <= now && now <= end;
  };
  const size_t count =
      std::min({foreground_objects.Size(), background_objects.Size(),
                next_objects.Size()});
  for (size_t i = 0; i < count; ++i) {
    const bool fg_exists = foreground_objects.Exists(i);
    const bool bg_exists = background_objects.Exists(i);

    const bool front_in_range =
        fg_exists && InWipeRange(foreground_objects.At(i).value().Param());
    const bool back_participates =
        bg_exists && (background_objects[i].has_object_data() ||
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

LazyArray<GraphicsObject>& Stage::ObjectsForLayer(int layer) {
  switch (layer) {
    case OBJ_FG:
      return foreground_objects;
    case OBJ_BG:
      return background_objects;
    case OBJ_NEXT:
      return next_objects;
    default:
      throw std::runtime_error("Invalid layer number");
  }
}

const LazyArray<GraphicsObject>& Stage::ObjectsForLayer(int layer) const {
  switch (layer) {
    case OBJ_FG:
      return foreground_objects;
    case OBJ_BG:
      return background_objects;
    case OBJ_NEXT:
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
  for (GraphicsObject& object : foreground_objects)
    object.FreeObjectData();

  for (GraphicsObject& object : background_objects)
    object.FreeObjectData();
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

LazyArray<GraphicsObject>& Stage::GetBackgroundObjects() {
  return background_objects;
}

LazyArray<GraphicsObject>& Stage::GetForegroundObjects() {
  return foreground_objects;
}

LazyArray<GraphicsObject>& Stage::GetNextObjects() { return next_objects; }

bool Stage::AnimationsPlaying() const {
  for (size_t i = 0, end = foreground_objects.Size(); i < end; ++i) {
    const auto& object = foreground_objects.At(i);
    if (object && object->has_object_data()) {
      const GraphicsObjectData& data = object->GetObjectData();
      if (data.IsAnimation() && data.GetAnimator()->IsPlaying())
        return true;
    }
  }

  return false;
}

void Stage::RenderObjects(const ObjectRenderPredicate& should_render) {
  to_render_.clear();

  for (auto it = foreground_objects.begin(), end = foreground_objects.end();
       it != end; ++it) {
    if (should_render && !should_render(it.pos(), *it))
      continue;

    to_render_.emplace_back(it->Param().z_order, it->Param().z_layer,
                            it->Param().z_depth, static_cast<int>(it.pos()),
                            &*it);
  }

  std::sort(to_render_.begin(), to_render_.end());

  for (auto& object : to_render_)
    std::get<4>(object)->Render(std::get<3>(object), nullptr);
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
