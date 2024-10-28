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

#include "object/drawer/text.h"

#include <ostream>
#include <vector>

#include "base/gameexe.hpp"
#include "systems/base/graphics_object.h"
#include "systems/base/surface.h"
#include "systems/base/system.h"
#include "systems/base/text_system.h"

// -----------------------------------------------------------------------

GraphicsTextObject::GraphicsTextObject(System& system)
    : system_(system), cached_param_(), surface_(nullptr) {}

// -----------------------------------------------------------------------

GraphicsTextObject::~GraphicsTextObject() {}

// -----------------------------------------------------------------------

void GraphicsTextObject::UpdateSurface(const GraphicsObject& rp) {
  auto text_property = rp.Param().Get<ObjectProperty::TextProperties>();

  cached_param_ = text_property;

  // Get the correct colour
  Gameexe& gexe = system_.gameexe();
  std::vector<int> vec = gexe("COLOR_TABLE", text_property.colour);
  RGBColour colour(vec.at(0), vec.at(1), vec.at(2));

  RGBColour* shadow = nullptr;
  RGBColour shadow_impl;
  if (text_property.shadow_colour != -1) {
    vec = gexe("COLOR_TABLE", text_property.shadow_colour);
    shadow_impl = RGBColour(vec.at(0), vec.at(1), vec.at(2));
    shadow = &shadow_impl;
  }

  surface_ = system_.text().RenderText(
      text_property.value, text_property.text_size, text_property.xspace,
      text_property.yspace, colour, shadow, text_property.char_count);
  surface_->EnsureUploaded();
}

// -----------------------------------------------------------------------

bool GraphicsTextObject::NeedsUpdate(const GraphicsObject& rp) {
  if (surface_ == nullptr)
    return true;
  return cached_param_ != rp.Param().Get<ObjectProperty::TextProperties>();
}

// -----------------------------------------------------------------------

std::shared_ptr<const Surface> GraphicsTextObject::CurrentSurface(
    const GraphicsObject& go) {
  if (NeedsUpdate(go))
    UpdateSurface(go);

  return surface_;
}

// -----------------------------------------------------------------------

int GraphicsTextObject::PixelWidth(const GraphicsObject& rp) {
  auto& param = rp.Param();

  if (NeedsUpdate(rp))
    UpdateSurface(rp);

  return int(param.GetWidthScaleFactor() * surface_->GetSize().width());
}

// -----------------------------------------------------------------------

int GraphicsTextObject::PixelHeight(const GraphicsObject& rp) {
  auto& param = rp.Param();

  if (NeedsUpdate(rp))
    UpdateSurface(rp);

  return int(param.GetHeightScaleFactor() * surface_->GetSize().height());
}

// -----------------------------------------------------------------------

std::unique_ptr<GraphicsObjectData> GraphicsTextObject::Clone() const {
  return std::make_unique<GraphicsTextObject>(*this);
}

// -----------------------------------------------------------------------

void GraphicsTextObject::Execute(RLMachine& machine) {}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsTextObject::load(Archive& ar, unsigned int version) {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this);

  surface_ = nullptr;
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsTextObject::save(Archive& ar, unsigned int version) const {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this);
}

// -----------------------------------------------------------------------

BOOST_CLASS_EXPORT(GraphicsTextObject);

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void GraphicsTextObject::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void GraphicsTextObject::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
