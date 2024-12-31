// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
// Copyright (C) 2000 Kazunori Ueno(JAGARL) <jagarl@creator.club.ne.jp>
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

#include "object/drawer/anm.hpp"

#include "systems/base/graphics_object.hpp"
#include "systems/sdl_surface.hpp"
#include "utilities/clock.hpp"

#include <iterator>
#include <vector>

// -----------------------------------------------------------------------
// AnmGraphicsObjectData
// -----------------------------------------------------------------------

AnmGraphicsObjectData::AnmGraphicsObjectData(std::shared_ptr<Surface> surface,
                                             AnmDecoder anm_data)
    : animator_(std::make_shared<Clock>()),
      frames(std::move(anm_data.frames)),
      framelist_(std::move(anm_data.framelist_)),
      animation_set_(std::move(anm_data.animation_set_)),
      image_(surface),
      current_set_(-1),
      delta_time_(0) {}

AnmGraphicsObjectData::~AnmGraphicsObjectData() = default;

void AnmGraphicsObjectData::Execute(RLMachine&) {
  if (animator_.IsPlaying())
    AdvanceFrame();
}

void AnmGraphicsObjectData::AdvanceFrame() {
  // Do things that advance the state
  delta_time_ += static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          animator_.GetDeltaTime())
          .count());
  bool done = false;

  while (animator_.IsPlaying() && !done) {
    if (delta_time_ > frames[current_frame_].time) {
      delta_time_ -= frames[current_frame_].time;

      cur_frame_++;
      if (cur_frame_ == cur_frame_end_) {
        cur_frame_set_++;
        if (cur_frame_set_ == cur_frame_set_end_) {
          GetAnimator()->SetIsPlaying(false);
        } else {
          cur_frame_ = framelist_.at(*cur_frame_set_).begin();
          cur_frame_end_ = framelist_.at(*cur_frame_set_).end();
          current_frame_ = *cur_frame_;
        }
      } else {
        current_frame_ = *cur_frame_;
      }
    } else {
      done = true;
    }
  }
}

// I am not entirely sure these methods even make sense given the
// context...
int AnmGraphicsObjectData::PixelWidth(const GraphicsObject& rp) {
  const GrpRect& rect = image_->GetPattern(rp.Param().GetPattNo());
  int width = rect.rect.width();
  return int(rp.Param().GetWidthScaleFactor() * width);
}

int AnmGraphicsObjectData::PixelHeight(const GraphicsObject& rp) {
  const GrpRect& rect = image_->GetPattern(rp.Param().GetPattNo());
  int height = rect.rect.height();
  return int(rp.Param().GetHeightScaleFactor() * height);
}

std::unique_ptr<GraphicsObjectData> AnmGraphicsObjectData::Clone() const {
  return std::make_unique<AnmGraphicsObjectData>(*this);
}

void AnmGraphicsObjectData::PlaySet(int set) {
  animator_.Reset();

  cur_frame_set_ = animation_set_.at(set).begin();
  cur_frame_set_end_ = animation_set_.at(set).end();
  cur_frame_ = framelist_.at(*cur_frame_set_).begin();
  cur_frame_end_ = framelist_.at(*cur_frame_set_).end();
  current_frame_ = *cur_frame_;
}

std::shared_ptr<const Surface> AnmGraphicsObjectData::CurrentSurface(
    const GraphicsObject& rp) {
  return image_;
}

Rect AnmGraphicsObjectData::SrcRect(const GraphicsObject& go) {
  if (current_frame_ != -1) {
    const Frame& frame = frames.at(current_frame_);
    return Rect::GRP(frame.src_x1, frame.src_y1, frame.src_x2, frame.src_y2);
  }

  return Rect();
}

Rect AnmGraphicsObjectData::DstRect(const GraphicsObject& go,
                                    const GraphicsObject* parent) {
  if (current_frame_ != -1) {
    // TODO(erg): Should this account for either |go| or |parent|?
    const Frame& frame = frames.at(current_frame_);
    return Rect::REC(frame.dest_x, frame.dest_y, (frame.src_x2 - frame.src_x1),
                     (frame.src_y2 - frame.src_y1));
  }

  return Rect();
}

// template <class Archive>
// void AnmGraphicsObjectData::load(Archive& ar, unsigned int version) {
//   ar & animator_ & filename_;

//   // Reconstruct the ANM data from whatever file was linked.
//   LoadAnmFile();

//   // Now load the rest of the data.
//   ar & currently_playing_ & current_set_;

//   // Reconstruct the cur_* variables from their
//   int cur_frame_set, current_frame;
//   ar & cur_frame_set & current_frame;

//   cur_frame_set_ = animation_set_.at(current_set_).begin();
//   advance(cur_frame_set_, cur_frame_set);
//   cur_frame_set_end_ = animation_set_.at(current_set_).end();

//   cur_frame_ = framelist_.at(*cur_frame_set_).begin();
//   advance(cur_frame_, current_frame);
//   cur_frame_end_ = framelist_.at(*cur_frame_set_).end();
// }

// template <class Archive>
// void AnmGraphicsObjectData::save(Archive& ar, unsigned int version) const {
//   ar & animator_ & filename_ & currently_playing_ & current_set_;

//   // Figure out what set we're playing, which
//   int cur_frame_set =
//       distance(animation_set_.at(current_set_).begin(), cur_frame_set_);
//   int current_frame =
//       distance(framelist_.at(*cur_frame_set_).begin(), cur_frame_);

//   ar & cur_frame_set & current_frame;
// }

// // -----------------------------------------------------------------------

// template void AnmGraphicsObjectData::save<boost::archive::text_oarchive>(
//     boost::archive::text_oarchive& ar,
//     unsigned int version) const;

// template void AnmGraphicsObjectData::load<boost::archive::text_iarchive>(
//     boost::archive::text_iarchive& ar,
//     unsigned int version);

// BOOST_CLASS_EXPORT(AnmGraphicsObjectData);
