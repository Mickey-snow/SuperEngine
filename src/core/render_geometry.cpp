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

#include "core/render_geometry.hpp"

#include "core/object.hpp"
#include "core/object_internal/objdrawer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

inline float deg2rad(float degrees) {
  return degrees * std::numbers::pi_v<float> / 180.0f;
}

Rect RectFromFloats(float x1, float y1, float x2, float y2) {
  const float left = std::min(x1, x2);
  const float right = std::max(x1, x2);
  const float top = std::min(y1, y2);
  const float bottom = std::max(y1, y2);
  return Rect::GRP(static_cast<int>(left), static_cast<int>(top),
                   static_cast<int>(right), static_cast<int>(bottom));
}

}  // namespace

std::pair<float, float> RotateAround(float x,
                                     float y,
                                     float center_x,
                                     float center_y,
                                     float degrees) {
  const float radians = deg2rad(degrees);
  const float cosv = std::cos(radians);
  const float sinv = std::sin(radians);
  const float dx = x - center_x;
  const float dy = y - center_y;
  return {dx * cosv - dy * sinv + center_x, dx * sinv + dy * cosv + center_y};
}

RenderState RenderState::BuildFrom(const GraphicsObject& go) {
  const auto& param = go.Param();
  const Point position = go.GetObjectData().DstPosition(go);
  return RenderState{
      .pos_x = static_cast<float>(position.x()),
      .pos_y = static_cast<float>(position.y()),
      .center_x = static_cast<float>(param.origin_x),
      .center_y = static_cast<float>(param.origin_y),
      .center_rep_x = static_cast<float>(param.rep_origin_x()),
      .center_rep_y = static_cast<float>(param.rep_origin_y()),
      .scale_x = param.GetWidthScaleFactor(),
      .scale_y = param.GetHeightScaleFactor(),
      .rotation_degrees = param.rotation() / 10.0f,
  };
}

RenderState RenderState::Fold(const RenderState& self,
                              const RenderState& parent) {
  RenderState state = self;
  state.pos_x = (state.pos_x - parent.center_rep_x) * parent.scale_x +
                parent.center_rep_x;
  state.pos_y = (state.pos_y - parent.center_rep_y) * parent.scale_y +
                parent.center_rep_y;

  auto [rotated_x, rotated_y] =
      RotateAround(state.pos_x, state.pos_y, parent.center_rep_x,
                   parent.center_rep_y, parent.rotation_degrees);
  state.pos_x = rotated_x + parent.pos_x;
  state.pos_y = rotated_y + parent.pos_y;
  state.scale_x *= parent.scale_x;
  state.scale_y *= parent.scale_y;
  state.rotation_degrees += parent.rotation_degrees;
  return state;
}

bool RenderGeometry::ApplySrcClip(const Rect clip) {
  const float local_left = local_x;
  const float local_top = local_y;
  const float local_right = local_left + src.width();
  const float local_bottom = local_top + src.height();

  const float clipped_left = std::max(local_left, static_cast<float>(clip.x()));
  const float clipped_top = std::max(local_top, static_cast<float>(clip.y()));
  const float clipped_right =
      std::min(local_right, static_cast<float>(clip.x2()));
  const float clipped_bottom =
      std::min(local_bottom, static_cast<float>(clip.y2()));

  if (clipped_right <= clipped_left || clipped_bottom <= clipped_top)
    return false;

  const int src_dx = static_cast<int>(clipped_left - local_left);
  const int src_dy = static_cast<int>(clipped_top - local_top);
  const int src_width = static_cast<int>(clipped_right - clipped_left);
  const int src_height = static_cast<int>(clipped_bottom - clipped_top);

  src = Rect::REC(src.x() + src_dx, src.y() + src_dy, src_width, src_height);
  local_x = clipped_left;
  local_y = clipped_top;
  UpdateDstFromLocal();
  return !src.is_degenerate() && !dst.is_degenerate();
}

bool RenderGeometry::ApplyDstClip(const Rect clip) {
  const Rect old_dst = dst;
  const Rect clipped = old_dst.Intersection(clip);
  if (clipped.is_degenerate())
    return false;

  if (old_dst.width() != 0) {
    const float src_per_dst_x =
        static_cast<float>(src.width()) / old_dst.width();
    const int src_dx =
        static_cast<int>((clipped.x() - old_dst.x()) * src_per_dst_x);
    const int src_width = static_cast<int>(clipped.width() * src_per_dst_x);
    src.set_x(src.x() + src_dx);
    src.set_x2(src.x() + src_width);
  }

  if (old_dst.height() != 0) {
    const float src_per_dst_y =
        static_cast<float>(src.height()) / old_dst.height();
    const int src_dy =
        static_cast<int>((clipped.y() - old_dst.y()) * src_per_dst_y);
    const int src_height = static_cast<int>(clipped.height() * src_per_dst_y);
    src.set_y(src.y() + src_dy);
    src.set_y2(src.y() + src_height);
  }

  dst = clipped;
  return !src.is_degenerate();
}

void RenderGeometry::UpdateDstFromLocal() {
  const float x1 = pivot_x + local_x * scale_x;
  const float y1 = pivot_y + local_y * scale_y;
  const float x2 = pivot_x + (local_x + src.width()) * scale_x;
  const float y2 = pivot_y + (local_y + src.height()) * scale_y;
  dst = RectFromFloats(x1, y1, x2, y2);
}
