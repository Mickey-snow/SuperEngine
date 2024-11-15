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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "libreallive/alldefs.hpp"
#include "machine/serialization.hpp"
#include "object/animator.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/surface.hpp"
#include "systems/base/system.hpp"
#include "utilities/exception.hpp"
#include "utilities/file.hpp"

using libreallive::read_i32;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ostringstream;
using std::string;
using std::vector;

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// GanGraphicsObjectData
// -----------------------------------------------------------------------

GanGraphicsObjectData::GanGraphicsObjectData(System& system)
    : system_(system),
      animator_(system.event().GetClock()),
      current_set_(-1),
      current_frame_(-1),
      delta_time_(0) {}

GanGraphicsObjectData::GanGraphicsObjectData(System& system,
                                             const std::string& gan_file,
                                             const std::string& img_file)
    : system_(system),
      animator_(system.event().GetClock()),
      gan_filename_(gan_file),
      img_filename_(img_file),
      current_set_(-1),
      current_frame_(-1),
      delta_time_(0) {
  LoadGANData();
}

GanGraphicsObjectData::~GanGraphicsObjectData() {}

void GanGraphicsObjectData::LoadGANData() {
  image_ = system_.graphics().GetSurfaceNamed(img_filename_);
  image_->EnsureUploaded();

  fs::path gan_file_path = system_.FindFile(gan_filename_, GAN_FILETYPES);
  if (gan_file_path.empty()) {
    ostringstream oss;
    oss << "Could not find GAN file \"" << gan_filename_ << "\".";
    throw rlvm::Exception(oss.str());
  }

  int file_size = 0;
  std::unique_ptr<char[]> gan_data;
  if (LoadFileData(gan_file_path, gan_data, file_size)) {
    ostringstream oss;
    oss << "Could not read the contents of \"" << gan_file_path << "\"";
    throw rlvm::Exception(oss.str());
  }

  TestFileMagic(gan_filename_, gan_data, file_size);
  ReadData(gan_filename_, gan_data, file_size);
}

void GanGraphicsObjectData::TestFileMagic(const std::string& file_name,
                                          std::unique_ptr<char[]>& gan_data,
                                          int file_size) {
  const char* data = gan_data.get();
  int a = read_i32(data);
  int b = read_i32(data + 0x04);
  int c = read_i32(data + 0x08);

  if (a != 10000 || b != 10000 || c != 10100)
    ThrowBadFormat(file_name, "Incorrect GAN file magic");
}

void GanGraphicsObjectData::ReadData(const std::string& file_name,
                                     std::unique_ptr<char[]>& gan_data,
                                     int file_size) {
  const char* data = gan_data.get();
  int file_name_length = read_i32(data + 0xc);
  string raw_file_name = data + 0x10;

  // Strings should be NULL terminated.
  data = data + 0x10 + file_name_length - 1;
  if (*data != 0)
    ThrowBadFormat(file_name, "Incorrect filename length in GAN header");
  data++;

  int twenty_thousand = read_i32(data);
  if (twenty_thousand != 20000)
    ThrowBadFormat(file_name, "Expected start of GAN data section");
  data += 4;

  int number_of_sets = read_i32(data);
  data += 4;

  for (int i = 0; i < number_of_sets; ++i) {
    int start_of_ganset = read_i32(data);
    if (start_of_ganset != 0x7530)
      ThrowBadFormat(file_name, "Expected start of GAN set");

    data += 4;
    int frame_count = read_i32(data);
    if (frame_count < 0)
      ThrowBadFormat(file_name,
                     "Expected animation to contain at least one frame");
    data += 4;

    vector<Frame> animation_set;
    for (int j = 0; j < frame_count; ++j)
      animation_set.push_back(ReadSetFrame(file_name, data));
    animation_sets.push_back(animation_set);
  }
}

GanGraphicsObjectData::Frame GanGraphicsObjectData::ReadSetFrame(
    const std::string& file_name,
    const char*& data) {
  GanGraphicsObjectData::Frame frame;

  int tag = read_i32(data);
  data += 4;
  while (tag != 999999) {
    int value = read_i32(data);
    data += 4;

    switch (tag) {
      case 30100:
        frame.pattern = value;
        break;
      case 30101:
        frame.x = value;
        break;
      case 30102:
        frame.y = value;
        break;
      case 30103:
        frame.time = value;
        break;
      case 30104:
        frame.alpha = value;
        break;
      case 30105:
        frame.other = value;
        break;
      default: {
        ostringstream oss;
        oss << "Unknown GAN frame tag: " << tag;
        ThrowBadFormat(file_name, oss.str());
      }
    }

    tag = read_i32(data);
    data += 4;
  }

  return frame;
}

void GanGraphicsObjectData::ThrowBadFormat(const std::string& file_name,
                                           const std::string& error) {
  ostringstream oss;
  oss << "File \"" << file_name
      << "\" does not appear to be in GAN format: " << error;
  throw rlvm::Exception(oss.str());
}

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
  return std::make_unique<GanGraphicsObjectData>(system_, gan_filename_,
                                                 img_filename_);
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
    const vector<Frame>& current_set = animation_sets.at(current_set_);
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

      system_.graphics().MarkScreenAsDirty(GUT_DISPLAY_OBJ);
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
  system_.graphics().MarkScreenAsDirty(GUT_DISPLAY_OBJ);
}

template <class Archive>
void GanGraphicsObjectData::load(Archive& ar, unsigned int version) {
  ar & animator_ & gan_filename_ & img_filename_ & current_set_ &
      current_frame_;

  LoadGANData();
  system_.graphics().MarkScreenAsDirty(GUT_DISPLAY_OBJ);
}

template <class Archive>
void GanGraphicsObjectData::save(Archive& ar, unsigned int version) const {
  ar & animator_ & gan_filename_ & img_filename_ & current_set_ &
      current_frame_;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void GanGraphicsObjectData::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void GanGraphicsObjectData::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);

// -----------------------------------------------------------------------

BOOST_CLASS_EXPORT(GanGraphicsObjectData);
