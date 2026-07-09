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

#pragma once

#include "core/object.hpp"
#include "utilities/lazy_array.hpp"

#include <cstddef>
#include <deque>
#include <string>

[[maybe_unused]] constexpr int kLayerFg = 0;
[[maybe_unused]] constexpr int kLayerBg = 1;
[[maybe_unused]] constexpr int kLayerNext = 2;

class Stage {
 public:
  Stage(int size);

  // Foreground objects
  LazyArray<GraphicsObject> foreground_objects;

  // Background objects
  LazyArray<GraphicsObject> background_objects;

  // Next objects
  LazyArray<GraphicsObject> next_objects;

  // Foreground objects (at the time of the last save)
  LazyArray<GraphicsObject> saved_foreground_objects;

  // Background objects (at the time of the last save)
  LazyArray<GraphicsObject> saved_background_objects;

  // List of commands in RealLive bytecode to rebuild the graphics stack at the
  // current moment.
  std::deque<std::string> graphics_stack;

  // Commands to rebuild the graphics stack (at the time of the last savepoint)
  std::deque<std::string> saved_graphics_stack;

 public:
  void Reset();

  // A process where the front and back buffers swap, updating the display to
  // show objects prepared in the back buffer. Documented as "Wipe operation".
  void Wipe();
  void Wipe(int begin_order, int end_order, int begin_layer, int end_layer);

  double foreground_render_alpha() const { return foreground_render_alpha_; }
  double next_render_alpha() const { return next_render_alpha_; }
  void SetTransitionRenderAlpha(double foreground_alpha, double next_alpha);
  void ClearTransitionRenderState();

  // Object getters
  // layer == OBJ_FG for foreground, OBJ_BG for background, OBJ_NEXT for next.
  GraphicsObject& GetObject(int layer, int obj_number);
  size_t GetFreeObjectId(int layer);
  void SetObject(int layer, int obj_number, GraphicsObject&& object);

  // Remove the entire graphics object.
  void RemoveObject(int layer, size_t obj_number);

  // Frees the object data (but not the parameters).
  void FreeObjectData(int obj_number);
  void FreeLayerObjectData(int layer);
  void FreeAllObjectData();

  // Resets/reinitializes all the object parameters without deleting the loaded
  // graphics object data.
  void InitializeObjectParams(int obj_number);
  void InitializeAllObjectParams();

  void Execute();

  // Adds |command|, the serialized form of a bytecode used by calling the
  // BytecodeElement::data().
  void AddGraphicsStackCommand(std::string command);

  // Takes a snapshot of the current object state. This snapshot is saved
  // instead of the current state of the graphics, since RealLive is a savepoint
  // based system.
  //
  // (This operation isn't exceptionally expensive; internally GraphicsObject
  // has multiple copy-on-write data structs to make this and object promotion a
  // relativly cheap operation.)
  void TakeSavepointSnapshot();

  const LazyArray<GraphicsObject>& ObjectsForLayer(int layer) const;
  LazyArray<GraphicsObject>& ObjectsForLayer(int layer);

 private:
  double foreground_render_alpha_ = 1.0;
  double next_render_alpha_ = 0.0;
};
