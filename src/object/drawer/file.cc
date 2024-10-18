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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include "object/drawer/file.h"

#include <iostream>
#include <memory>
#include <string>

#include "machine/serialization.h"
#include "systems/base/event_system.h"
#include "systems/base/graphics_object.h"
#include "systems/base/graphics_system.h"
#include "systems/base/surface.h"
#include "systems/base/system.h"
#include "utilities/file.h"

namespace fs = std::filesystem;

// -----------------------------------------------------------------------

GraphicsObjectOfFile::GraphicsObjectOfFile(System& system)
    : service_(std::make_shared<RenderingService>(system)),
      filename_(""),
      frame_time_(0),
      current_frame_(0),
      time_at_last_frame_change_(0) {}

// -----------------------------------------------------------------------

GraphicsObjectOfFile::GraphicsObjectOfFile(System& system,
                                           const std::string& filename)
    : service_(std::make_shared<RenderingService>(system)),
      filename_(filename),
      frame_time_(0),
      current_frame_(0),
      time_at_last_frame_change_(0) {
  LoadFile(system);
}

// -----------------------------------------------------------------------

GraphicsObjectOfFile::~GraphicsObjectOfFile() {}

// -----------------------------------------------------------------------

void GraphicsObjectOfFile::LoadFile(System& system) {
  surface_ = system.graphics().GetSurfaceNamed(filename_);
  surface_->EnsureUploaded();
}

// -----------------------------------------------------------------------

int GraphicsObjectOfFile::PixelWidth(const GraphicsObject& rp) {
  auto& param = rp.Param();

  const GrpRect& rect = surface_->GetPattern(param.GetPattNo());
  int width = rect.rect.width();
  return int(param.GetWidthScaleFactor() * width);
}

// -----------------------------------------------------------------------

int GraphicsObjectOfFile::PixelHeight(const GraphicsObject& rp) {
  auto& param = rp.Param();

  const GrpRect& rect = surface_->GetPattern(param.GetPattNo());
  int height = rect.rect.height();
  return int(param.GetHeightScaleFactor() * height);
}

// -----------------------------------------------------------------------

std::unique_ptr<GraphicsObjectData> GraphicsObjectOfFile::Clone() const {
  return std::make_unique<GraphicsObjectOfFile>(*this);
}

// -----------------------------------------------------------------------

void GraphicsObjectOfFile::Execute(RLMachine&) {
  if (GetAnimator()->IsPlaying()) {
    unsigned int current_time = service_->GetTicks();
    unsigned int time_since_last_frame_change =
        current_time - time_at_last_frame_change_;

    while (time_since_last_frame_change > frame_time_) {
      current_frame_++;
      if (current_frame_ == surface_->GetNumPatterns()) {
        current_frame_--;
        EndAnimation();
      }

      time_at_last_frame_change_ += frame_time_;
      time_since_last_frame_change = current_time - time_at_last_frame_change_;
      service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
    }
  }
}

// -----------------------------------------------------------------------

bool GraphicsObjectOfFile::IsAnimation() const {
  return surface_->GetNumPatterns();
}

// -----------------------------------------------------------------------

void GraphicsObjectOfFile::LoopAnimation() { current_frame_ = 0; }

// -----------------------------------------------------------------------

std::shared_ptr<const Surface> GraphicsObjectOfFile::CurrentSurface(
    const GraphicsObject& rp) {
  return surface_;
}

// -----------------------------------------------------------------------

Rect GraphicsObjectOfFile::SrcRect(const GraphicsObject& go) {
  if (time_at_last_frame_change_ != 0) {
    // If we've ever been treated as an animation, we need to continue acting
    // as an animation even if we've stopped.
    return surface_->GetPattern(current_frame_).rect;
  }

  return GraphicsObjectData::SrcRect(go);
}

// -----------------------------------------------------------------------

void GraphicsObjectOfFile::PlaySet(int frame_time) {
  GetAnimator()->SetIsPlaying(true);
  frame_time_ = frame_time;
  current_frame_ = 0;

  if (frame_time_ == 0) {
    std::cerr << "WARNING: GraphicsObjectOfFile::PlaySet(0) is invalid;"
              << " this is probably going to cause a graphical glitch..."
              << std::endl;
    frame_time_ = 10;
  }

  time_at_last_frame_change_ = service_->GetTicks();
  service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsObjectOfFile::load(Archive& ar, unsigned int version) {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this) & filename_ &
      frame_time_ & current_frame_ & time_at_last_frame_change_;

  LoadFile(Serialization::g_current_machine->system());

  // Saving |time_at_last_frame_change_| as part of the format is obviously a
  // mistake, but is now baked into the file format. Ask the clock for a more
  // suitable value.
  if (time_at_last_frame_change_ != 0) {
    time_at_last_frame_change_ = service_->GetTicks();
    service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
  }
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsObjectOfFile::save(Archive& ar, unsigned int version) const {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this) & filename_ &
      frame_time_ & current_frame_ & time_at_last_frame_change_;
}

// -----------------------------------------------------------------------

BOOST_CLASS_EXPORT(GraphicsObjectOfFile);

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void GraphicsObjectOfFile::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void GraphicsObjectOfFile::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
