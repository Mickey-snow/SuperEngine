// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

// This code is heavily based off Haeleth's O'caml implementation
// (which translates binary GAN files to and from an XML
// representation), found at rldev/src/rlxml/gan.ml.

#include "object/drawer/gan.hpp"

#include "object/animator.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/sdl_surface.hpp"
#include "utilities/clock.hpp"

// -----------------------------------------------------------------------
// GanGraphicsObjectData
// -----------------------------------------------------------------------

GanGraphicsObjectData::GanGraphicsObjectData(
    std::shared_ptr<Surface> image,
    std::vector<std::vector<GanDecoder::Frame>> frames)
    : animator_(std::make_shared<Clock>()),
      image_(image),
      animation_sets(std::move(frames)),
      current_set_(-1),
      current_frame_(-1),
      delta_time_(0) {}

GanGraphicsObjectData::~GanGraphicsObjectData() = default;

int GanGraphicsObjectData::PixelWidth(const GraphicsObject& go) {
  auto& rendering_properties = go.Param();

  if (current_set_ != -1 && current_frame_ != -1) {
    const Frame& frame = animation_sets.at(current_set_).at(current_frame_);
    if (frame.pattern != -1) {
      const GrpRect& rect = image_->GetPattern(frame.pattern);
      return int(rendering_properties.GetWidthScaleFactor() *
                 rect.rect.width());
    }
  }

  return 0;
}

int GanGraphicsObjectData::PixelHeight(const GraphicsObject& go) {
  auto& rendering_properties = go.Param();

  if (current_set_ != -1 && current_frame_ != -1) {
    const Frame& frame = animation_sets.at(current_set_).at(current_frame_);
    if (frame.pattern != -1) {
      const GrpRect& rect = image_->GetPattern(frame.pattern);
      return int(rendering_properties.GetHeightScaleFactor() *
                 rect.rect.height());
    }
  }

  return 0;
}

std::unique_ptr<GraphicsObjectData> GanGraphicsObjectData::Clone() const {
  return std::make_unique<GanGraphicsObjectData>(*this);
}

void GanGraphicsObjectData::Execute(RLMachine&) {
  // obtain delta time first to avoid adding up when paused
  unsigned int deltaTime = static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          animator_.GetDeltaTime())
          .count());

  if (!animator_.IsPlaying())
    return;

  delta_time_ += deltaTime;
  if (current_frame_ >= 0) {
    const std::vector<Frame>& current_set = animation_sets.at(current_set_);
    const auto total_frames = current_set.size();

    while (delta_time_ >= current_set[current_frame_].time) {
      delta_time_ -= current_set[current_frame_++].time;

      if (current_frame_ >= total_frames) {
        if (animator_.GetAfterAction() == AFTER_LOOP)
          current_frame_ %= total_frames;
        else {
          delta_time_ = 0;
          current_frame_ = total_frames - 1;
          break;
        }
      }
    }
  }
}

std::shared_ptr<const Surface> GanGraphicsObjectData::CurrentSurface(
    const GraphicsObject&) {
  if (current_set_ != -1 && current_frame_ != -1) {
    const Frame& frame = animation_sets.at(current_set_).at(current_frame_);

    if (frame.pattern != -1) {
      // We are currently rendering an animation AND the current frame says to
      // render something to the screen.
      return image_;
    }
  }

  return std::shared_ptr<const Surface>();
}

Rect GanGraphicsObjectData::SrcRect(const GraphicsObject&) {
  const Frame& frame = animation_sets.at(current_set_).at(current_frame_);
  if (frame.pattern != -1) {
    return image_->GetPattern(frame.pattern).rect;
  }

  return Rect();
}

Point GanGraphicsObjectData::DstOrigin(const GraphicsObject& go) {
  const Frame& frame = animation_sets.at(current_set_).at(current_frame_);
  return GraphicsObjectData::DstOrigin(go) - Size(frame.x, frame.y);
}

int GanGraphicsObjectData::GetRenderingAlpha(const GraphicsObject& go,
                                             const GraphicsObject* parent) {
  auto& param = go.Param();

  const Frame& frame = animation_sets.at(current_set_).at(current_frame_);
  if (frame.pattern != -1) {
    // Calculate the combination of our frame alpha with the current object
    // alpha.
    float parent_alpha =
        parent ? (parent->Param().GetComputedAlpha() / 255.0f) : 1;
    return int(((frame.alpha / 255.0f) * (param.GetComputedAlpha() / 255.0f) *
                parent_alpha) *
               255);
  } else {
    // Should never happen.
    return param.GetComputedAlpha();
  }
}

void GanGraphicsObjectData::PlaySet(int set) {
  animator_.Reset();
  current_set_ = set;
  current_frame_ = 0;
}

// template <class Archive>
// void GanGraphicsObjectData::load(Archive& ar, unsigned int version) {
//   ar & animator_ & gan_filename_ & img_filename_ & current_set_ &
//       current_frame_;

//   LoadGANData();
// }

// template <class Archive>
// void GanGraphicsObjectData::save(Archive& ar, unsigned int version) const {
//   ar & animator_ & gan_filename_ & img_filename_ & current_set_ &
//       current_frame_;
// }

// // -----------------------------------------------------------------------

// // Explicit instantiations for text archives (since we hide the
// // implementation)

// template void GanGraphicsObjectData::save<boost::archive::text_oarchive>(
//     boost::archive::text_oarchive& ar,
//     unsigned int version) const;

// template void GanGraphicsObjectData::load<boost::archive::text_iarchive>(
//     boost::archive::text_iarchive& ar,
//     unsigned int version);

// // -----------------------------------------------------------------------

// BOOST_CLASS_EXPORT(GanGraphicsObjectData);
