// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
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

#include "core/object_internal/objdrawer.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"

#include "core/localrect.hpp"
#include "core/object.hpp"
#include "core/rect.hpp"
#include "systems/sdl/glrenderer.hpp"
#include "systems/sdl/sdl_surface.hpp"

#include <algorithm>
#include <cmath>
#include <ostream>

namespace {

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
};

inline float deg2rad(float degrees) {
  return degrees * std::numbers::pi_v<float> / 180.0f;
}

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

RenderState BuildState(const GraphicsObject& go, Point position) {
  const auto& param = go.Param();
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

void ApplyParentState(RenderState& state, const GraphicsObject& parent) {
  const auto& param = parent.Param();
  const float parent_pos_x =
      static_cast<float>(param.x() + param.GetXAdjustmentSum());
  const float parent_pos_y =
      static_cast<float>(param.y() + param.GetYAdjustmentSum());
  const float parent_center_rep_x = static_cast<float>(param.rep_origin_x());
  const float parent_center_rep_y = static_cast<float>(param.rep_origin_y());
  const float parent_scale_x = param.GetWidthScaleFactor();
  const float parent_scale_y = param.GetHeightScaleFactor();
  const float parent_rotation = param.rotation() / 10.0f;

  state.pos_x = (state.pos_x - parent_center_rep_x) * parent_scale_x +
                parent_center_rep_x;
  state.pos_y = (state.pos_y - parent_center_rep_y) * parent_scale_y +
                parent_center_rep_y;

  auto [rotated_x, rotated_y] =
      RotateAround(state.pos_x, state.pos_y, parent_center_rep_x,
                   parent_center_rep_y, parent_rotation);
  state.pos_x = rotated_x + parent_pos_x;
  state.pos_y = rotated_y + parent_pos_y;
  state.scale_x *= parent_scale_x;
  state.scale_y *= parent_scale_y;
  state.rotation_degrees += parent_rotation;
}

Rect RectFromFloats(float x1, float y1, float x2, float y2) {
  const float left = std::min(x1, x2);
  const float right = std::max(x1, x2);
  const float top = std::min(y1, y2);
  const float bottom = std::max(y1, y2);
  return Rect::GRP(static_cast<int>(left), static_cast<int>(top),
                   static_cast<int>(right), static_cast<int>(bottom));
}

glm::mat4 BuildModelMatrix(const GraphicsObjectData::RenderGeometry& geometry,
                           const Rect& dst) {
  glm::mat4 model(1.0f);
  model = glm::translate(model,
                         glm::vec3(geometry.pivot_x, geometry.pivot_y, 0.0f));
  model = glm::rotate(model, glm::radians(geometry.rotation_degrees),
                      glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::translate(model,
                         glm::vec3(-geometry.pivot_x, -geometry.pivot_y, 0.0f));
  model = glm::translate(model, glm::vec3(dst.x(), dst.y(), 0.0f));
  return model;
}

}  // namespace

bool GraphicsObjectData::RenderGeometry::ApplySrcClip(const Rect clip) {
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

bool GraphicsObjectData::RenderGeometry::ApplyDstClip(const Rect clip) {
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

void GraphicsObjectData::RenderGeometry::UpdateDstFromLocal() {
  const float x1 = pivot_x + local_x * scale_x;
  const float y1 = pivot_y + local_y * scale_y;
  const float x2 = pivot_x + (local_x + src.width()) * scale_x;
  const float y2 = pivot_y + (local_y + src.height()) * scale_y;
  dst = RectFromFloats(x1, y1, x2, y2);
}

// -----------------------------------------------------------------------
// GraphicsObjectData
// -----------------------------------------------------------------------

GraphicsObjectData::GraphicsObjectData() = default;

GraphicsObjectData::~GraphicsObjectData() = default;

void GraphicsObjectData::Render(const GraphicsObject& go,
                                const GraphicsObject* parent) {
  std::shared_ptr<const SDLSurface> surface = CurrentSurface(go);
  if (!surface)
    return;

  RenderGeometry geometry = BuildRenderGeometry(go, parent);
  int alpha = GetRenderingAlpha(go, parent);

  auto& param = go.Param();
  if (parent && parent->Param().has_own_clip_rect()) {
    // In Little Busters, a parent clip rect is used to clip text scrolling
    // in the battle system. rlvm has the concept of parent objects badly
    // hacked in, and that means we can't directly apply the own clip
    // rect. Instead we have to calculate this in terms of the screen
    // coordinates and then apply that as a global clip rect.
    Point parent_start(
        parent->Param().x() + parent->Param().GetXAdjustmentSum(),
        parent->Param().y() + parent->Param().GetYAdjustmentSum());
    Rect full_parent_clip =
        Rect(parent_start + parent->Param().own_clip_rect().origin(),
             parent->Param().own_clip_rect().size());

    if (!geometry.ApplyDstClip(full_parent_clip))
      return;
  }

  if (param.has_own_clip_rect()) {
    if (!geometry.ApplySrcClip(param.own_clip_rect()))
      return;
  }

  // Perform the object clipping.
  if (param.has_clip_rect()) {
    if (!geometry.ApplyDstClip(param.clip_rect()))
      return;
  }

  // surface->RenderToScreenAsObject(go, src, dst, alpha);
  for (SDLSurface::TextureRecord it : surface->GetTextureArray()) {
    auto src_rect = geometry.src, dst_rect = geometry.dst;
    LocalRect coordinate_system(it.x_, it.y_, it.w_, it.h_);
    if (!coordinate_system.intersectAndTransform(src_rect, dst_rect))
      continue;

    RenderingConfig config;
    config.alpha = alpha;
    config.model = BuildModelMatrix(geometry, dst_rect);
    config.blend_type = param.composite_mode;
    config.color = param.colour();
    config.tint = param.tint();
    config.mono = param.mono();
    config.invert = param.invert();
    config.light = param.light();

    glRenderer().Render({it.gltexture, src_rect}, std::move(config),
                        {SDLSurface::screen_, dst_rect});
  }
}

Rect GraphicsObjectData::SrcRect(const GraphicsObject& go) {
  return CurrentSurface(go)->GetPattern(go.Param().GetPattNo()).rect;
}

Point GraphicsObjectData::DstOrigin(const GraphicsObject& go) {
  auto& param = go.Param();
  std::shared_ptr<const SDLSurface> surface = CurrentSurface(go);
  if (surface) {
    return Point(surface->GetPattern(param.GetPattNo()).originX,
                 surface->GetPattern(param.GetPattNo()).originY);
  }

  return Point();
}

Rect GraphicsObjectData::DstRect(const GraphicsObject& go,
                                 const GraphicsObject* parent) {
  return BuildRenderGeometry(go, parent).dst;
}

bool GraphicsObjectData::HitTest(const GraphicsObject& go,
                                 const GraphicsObject* parent,
                                 const Point& point) {
  std::shared_ptr<const SDLSurface> surface = CurrentSurface(go);
  if (!surface)
    return false;

  RenderGeometry geometry = BuildRenderGeometry(go, parent);
  auto& param = go.Param();

  if (parent && parent->Param().has_own_clip_rect()) {
    Point parent_start(
        parent->Param().x() + parent->Param().GetXAdjustmentSum(),
        parent->Param().y() + parent->Param().GetYAdjustmentSum());
    Rect full_parent_clip =
        Rect(parent_start + parent->Param().own_clip_rect().origin(),
             parent->Param().own_clip_rect().size());
    if (!geometry.ApplyDstClip(full_parent_clip))
      return false;
  }

  if (param.has_own_clip_rect() &&
      !geometry.ApplySrcClip(param.own_clip_rect()))
    return false;

  if (param.has_clip_rect() && !geometry.ApplyDstClip(param.clip_rect()))
    return false;

  float hit_x = static_cast<float>(point.x());
  float hit_y = static_cast<float>(point.y());
  if (geometry.rotation_degrees != 0.0f) {
    auto [rotated_x, rotated_y] =
        RotateAround(hit_x, hit_y, geometry.pivot_x, geometry.pivot_y,
                     -geometry.rotation_degrees);
    hit_x = rotated_x;
    hit_y = rotated_y;
  }

  if (hit_x < geometry.dst.x() || hit_x >= geometry.dst.x2() ||
      hit_y < geometry.dst.y() || hit_y >= geometry.dst.y2())
    return false;

  if (!param.alpha_test)
    return true;

  if (geometry.dst.width() == 0 || geometry.dst.height() == 0)
    return false;

  const float source_x_ratio =
      (hit_x - geometry.dst.x()) / static_cast<float>(geometry.dst.width());
  const float source_y_ratio =
      (hit_y - geometry.dst.y()) / static_cast<float>(geometry.dst.height());
  int source_x = geometry.src.x() +
                 static_cast<int>(source_x_ratio *
                                  static_cast<float>(geometry.src.width()));
  int source_y = geometry.src.y() +
                 static_cast<int>(source_y_ratio *
                                  static_cast<float>(geometry.src.height()));
  source_x = std::clamp(source_x, geometry.src.x(), geometry.src.x2() - 1);
  source_y = std::clamp(source_y, geometry.src.y(), geometry.src.y2() - 1);

  const Point source_point(source_x, source_y);
  if (!surface->GetRect().Contains(source_point))
    return false;

  return surface->GetPixelAt(source_point).a() > 0;
}

Point GraphicsObjectData::DstPosition(const GraphicsObject& go) {
  auto& param = go.Param();
  Point position(param.x() + param.GetXAdjustmentSum(),
                 param.y() + param.GetYAdjustmentSum());
  if (param.GetButtonUsingOverides()) {
    position += Point(param.GetButtonXOffsetOverride(),
                      param.GetButtonYOffsetOverride());
  }
  return position;
}

GraphicsObjectData::RenderGeometry GraphicsObjectData::BuildRenderGeometry(
    const GraphicsObject& go,
    const GraphicsObject* parent) {
  RenderGeometry geometry;
  geometry.src = SrcRect(go);

  RenderState state = BuildState(go, DstPosition(go));
  if (parent)
    ApplyParentState(state, *parent);

  const Point texture_origin = DstOrigin(go);
  const float pivot_x = state.pos_x + state.center_rep_x;
  const float pivot_y = state.pos_y + state.center_rep_y;
  const float center_x =
      state.center_x + state.center_rep_x + texture_origin.x();
  const float center_y =
      state.center_y + state.center_rep_y + texture_origin.y();

  geometry.pivot_x = pivot_x;
  geometry.pivot_y = pivot_y;
  geometry.pivot = Point(static_cast<int>(pivot_x), static_cast<int>(pivot_y));
  geometry.rotation_degrees = state.rotation_degrees;
  geometry.scale_x = state.scale_x;
  geometry.scale_y = state.scale_y;
  geometry.local_x = -center_x;
  geometry.local_y = -center_y;
  geometry.UpdateDstFromLocal();

  return geometry;
}

int GraphicsObjectData::GetRenderingAlpha(const GraphicsObject& go,
                                          const GraphicsObject* parent) {
  const int alpha = go.Param().GetComputedAlpha();
  if (!parent) {
    return alpha;
  } else {
    const int par_alpha = parent->Param().GetComputedAlpha();
    return static_cast<int>((par_alpha / 255.f) * (alpha / 255.f) * 255);
  }
}

void GraphicsObjectData::PlaySet(int set) {}

void GraphicsObjectData::Execute() {}
