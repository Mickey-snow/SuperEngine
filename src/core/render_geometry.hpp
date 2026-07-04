// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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

#include <utility>

class GraphicsObject;

std::pair<float, float> RotateAround(float x,
                                     float y,
                                     float center_x,
                                     float center_y,
                                     float degrees);

struct RenderGeometry {
  Rect src;
  Rect dst;
  float pivot_x = 0.0f;
  float pivot_y = 0.0f;
  float rotation_degrees = 0.0f;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  float local_x = 0.0f;
  float local_y = 0.0f;

  void UpdateDstFromLocal();
  bool ApplySrcClip(const Rect clip);
  bool ApplyDstClip(const Rect clip);
};

struct RenderState {
  float pos_x = 0.0f;
  float pos_y = 0.0f;
  float center_x = 0.0f;
  float center_y = 0.0f;
  float center_rep_x = 0.0f;
  float center_rep_y = 0.0f;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  float rotation_degrees = 0.0f;

  static RenderState BuildFrom(const GraphicsObject& go);
  static RenderState Fold(const RenderState& self, const RenderState& parent);
};
