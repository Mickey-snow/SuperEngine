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

#include "systems/base/graphics_object_data.h"

#include <ostream>

#include "base/rect.h"
#include "systems/base/graphics_object.h"
#include "systems/base/graphics_object_of_file.h"
#include "systems/base/surface.h"

// -----------------------------------------------------------------------
// GraphicsObjectData
// -----------------------------------------------------------------------

GraphicsObjectData::GraphicsObjectData()
    : after_animation_(AFTER_NONE),
      owned_by_(NULL),
      currently_playing_(false),
      animation_finished_(false) {}

GraphicsObjectData::GraphicsObjectData(const GraphicsObjectData& obj)
    : after_animation_(obj.after_animation_),
      owned_by_(NULL),
      currently_playing_(obj.currently_playing_),
      animation_finished_(false) {}

GraphicsObjectData::~GraphicsObjectData() {}

void GraphicsObjectData::Render(const GraphicsObject& go,
                                const GraphicsObject* parent) {
  std::shared_ptr<const Surface> surface = CurrentSurface(go);
  if (surface) {
    Rect src = SrcRect(go);
    Rect dst = DstRect(go, parent);
    int alpha = GetRenderingAlpha(go, parent);

    auto& param = go.Param();
    if (param.GetButtonUsingOverides()) {
      // Tacked on side channel that lets a ButtonObjectSelectLongOperation
      // tweak the x/y coordinates of dst. There isn't really a better place to
      // put this. It can't go in dstRect() because the LongOperation also
      // consults the data from dstRect().
      dst = Rect(dst.origin() + Size(param.GetButtonXOffsetOverride(),
                                     param.GetButtonYOffsetOverride()),
                 dst.size());
    }

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

      Rect clipped_dest = dst.Intersection(full_parent_clip);
      Rect inset = dst.GetInsetRectangle(clipped_dest);
      dst = clipped_dest;
      src = src.ApplyInset(inset);
    }

    if (param.has_own_clip_rect()) {
      dst = dst.ApplyInset(param.own_clip_rect());
      src = src.ApplyInset(param.own_clip_rect());
    }

    // Perform the object clipping.
    if (param.has_clip_rect()) {
      Rect clipped_dest = dst.Intersection(param.clip_rect());

      // Do nothing if object falls wholly outside clip area
      if (clipped_dest.is_empty())
        return;

      // Adjust the source rectangle
      Rect inset = dst.GetInsetRectangle(clipped_dest);

      dst = clipped_dest;
      src = src.ApplyInset(inset);
    }

    // TODO(erg): Do we want to skip this if no alpha?
    surface->RenderToScreenAsObject(go, src, dst, alpha);
  }
}

void GraphicsObjectData::LoopAnimation() {}

void GraphicsObjectData::EndAnimation() {
  // Set first, because we may deallocate this by one of our actions
  currently_playing_ = false;

  switch (after_animation_) {
    case AFTER_NONE:
      animation_finished_ = true;
      break;
    case AFTER_CLEAR:
      if (owned_by_)
        owned_by_->FreeObjectData();
      break;
    case AFTER_LOOP: {
      // Reset from the beginning
      currently_playing_ = true;
      LoopAnimation();
      break;
    }
  }
}

void GraphicsObjectData::PrintStringVector(
    const std::vector<std::string>& names,
    std::ostream* oss) {
  bool first = true;

  for (auto const& name : names) {
    if (!first)
      *oss << ", ";
    else
      first = false;

    *oss << name;
  }
}

Rect GraphicsObjectData::SrcRect(const GraphicsObject& go) {
  return CurrentSurface(go)->GetPattern(go.Param().GetPattNo()).rect;
}

Point GraphicsObjectData::DstOrigin(const GraphicsObject& go) {
  auto& param = go.Param();
  if (param.origin_x() || param.origin_y()) {
    return Point(param.origin_x(), param.origin_y());
  }

  std::shared_ptr<const Surface> surface = CurrentSurface(go);
  if (surface) {
    return Point(surface->GetPattern(param.GetPattNo()).originX,
                 surface->GetPattern(param.GetPattNo()).originY);
  }

  return Point();
}

Rect GraphicsObjectData::DstRect(const GraphicsObject& go,
                                 const GraphicsObject* parent) {
  Point origin = DstOrigin(go);
  Rect src = SrcRect(go);
  auto& param = go.Param();

  int center_x =
      param.x() + param.GetXAdjustmentSum() - origin.x() + (src.width() / 2.0f);
  int center_y = param.y() + param.GetYAdjustmentSum() - origin.y() +
                 (src.height() / 2.0f);

  float second_factor_x = 1.0f;
  float second_factor_y = 1.0f;
  if (parent) {
    center_x += parent->Param().x() + parent->Param().GetXAdjustmentSum();
    center_y += parent->Param().y() + parent->Param().GetYAdjustmentSum();

    second_factor_x = parent->Param().GetWidthScaleFactor();
    second_factor_y = parent->Param().GetHeightScaleFactor();
  }

  int half_real_width =
      (src.width() * second_factor_x * param.GetWidthScaleFactor()) / 2.0f;
  int half_real_height =
      (src.height() * second_factor_y * param.GetHeightScaleFactor()) / 2.0f;

  int xPos1 = center_x - half_real_width;
  int yPos1 = center_y - half_real_height;
  int xPos2 = center_x + half_real_width;
  int yPos2 = center_y + half_real_height;

  return Rect::GRP(xPos1, yPos1, xPos2, yPos2);
}

int GraphicsObjectData::GetRenderingAlpha(const GraphicsObject& go,
                                          const GraphicsObject* parent) {
  auto& param = go.Param();

  if (!parent) {
    return param.GetComputedAlpha();
  } else {
    return int((parent->Param().GetComputedAlpha() / 255.0f) *
               (param.GetComputedAlpha() / 255.0f) * 255);
  }
}

bool GraphicsObjectData::IsAnimation() const { return false; }

void GraphicsObjectData::PlaySet(int set) {}

bool GraphicsObjectData::IsParentLayer() const { return false; }
