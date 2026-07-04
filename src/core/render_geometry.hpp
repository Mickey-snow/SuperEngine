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

#include <string>
#include <utility>

class ObjectParameter;

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
  float pos_x = 0.0f, pos_y = 0.0f;                // translate
  float center_x = 0.0f, center_y = 0.0f;          // actual center
  float center_rep_x = 0.0f, center_rep_y = 0.0f;  // pivot center
  float scale_x = 1.0f, scale_y = 1.0f;            // accumulated scale
  float rotation_degrees = 0.0f;                   // accumulated rotate

  std::string GetDebugString() const;
  inline operator std::string() { return GetDebugString(); }
  bool operator==(const RenderState&) const;

  static constexpr RenderState Id() { return {}; }
  static RenderState Build(const ObjectParameter& param, Point dst_pos);
  static RenderState Fold(RenderState child, RenderState parent);
};
