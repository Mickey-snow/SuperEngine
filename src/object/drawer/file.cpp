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

#include "object/drawer/file.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "machine/serialization.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/surface.hpp"
#include "systems/base/system.hpp"
#include "utilities/file.hpp"

namespace fs = std::filesystem;

// -----------------------------------------------------------------------

GraphicsObjectOfFile::GraphicsObjectOfFile(System& system)
    : service_(std::make_shared<RenderingService>(system)),
      animator_(system.event().GetClock()),
      filename_(""),
      frame_time_(0),
      current_frame_(-1) {}

// -----------------------------------------------------------------------

GraphicsObjectOfFile::GraphicsObjectOfFile(System& system,
                                           const std::string& filename)
    : service_(std::make_shared<RenderingService>(system)),
      animator_(system.event().GetClock()),
      filename_(filename),
      frame_time_(0),
      current_frame_(-1) {
  LoadFile(system);
}

// -----------------------------------------------------------------------

GraphicsObjectOfFile::~GraphicsObjectOfFile() = default;

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

void GraphicsObjectOfFile::Execute(RLMachine&) { Execute(); }
void GraphicsObjectOfFile::Execute() {
  if (!animator_.IsPlaying())
    return;

  auto anmtime = animator_.GetAnimationTime();
  size_t current_frame =
      std::chrono::duration_cast<std::chrono::milliseconds>(anmtime).count() /
      frame_time_;

  const auto total_frames = surface_->GetNumPatterns();
  if (current_frame >= total_frames) {
    if (animator_.GetAfterAction() == AFTER_LOOP)
      current_frame %= total_frames;
    else
      current_frame = total_frames - 1;
  }

  service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
}

// -----------------------------------------------------------------------

std::shared_ptr<const Surface> GraphicsObjectOfFile::CurrentSurface(
    const GraphicsObject& rp) {
  return surface_;
}

// -----------------------------------------------------------------------

Rect GraphicsObjectOfFile::SrcRect(const GraphicsObject& go) {
  if (current_frame_ >= 0) {
    // If we've ever been treated as an animation, we need to continue acting
    // as an animation even if we've stopped.
    return surface_->GetPattern(current_frame_).rect;
  }

  return GraphicsObjectData::SrcRect(go);
}

// -----------------------------------------------------------------------

void GraphicsObjectOfFile::PlaySet(int frame_time) {
  frame_time_ = frame_time;
  current_frame_ = 0;

  if (frame_time_ == 0) {
    std::cerr << "WARNING: GraphicsObjectOfFile::PlaySet(0) is invalid;"
              << " this is probably going to cause a graphical glitch..."
              << std::endl;
    frame_time_ = 10;
  }

  animator_.Reset();
  service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
}

// -----------------------------------------------------------------------

Animator const* GraphicsObjectOfFile::GetAnimator() const {
  if (surface_->GetNumPatterns() <= 0)
    return nullptr;
  return &animator_;
}

// -----------------------------------------------------------------------

Animator* GraphicsObjectOfFile::GetAnimator() {
  if (surface_->GetNumPatterns() <= 0)
    return nullptr;
  return &animator_;
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsObjectOfFile::load(Archive& ar, unsigned int version) {
  ar & animator_ & filename_ & frame_time_ & current_frame_;

  LoadFile(Serialization::g_current_machine->GetSystem());
  service_->MarkScreenDirty(GUT_DISPLAY_OBJ);
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsObjectOfFile::save(Archive& ar, unsigned int version) const {
  ar & animator_ & filename_ & frame_time_ & current_frame_;
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
