// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2011 Elliot Glaysher
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
// -----------------------------------------------------------------------

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include "object/drawer/colour_filter.hpp"

#include <iostream>
#include <ostream>

#include "base/colour.hpp"
#include "systems/base/colour_filter.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "utilities/exception.hpp"

ColourFilterObjectData::ColourFilterObjectData(GraphicsSystem& system,
                                               const Rect& screen_rect)
    : graphics_system_(system), screen_rect_(screen_rect) {}

ColourFilterObjectData::~ColourFilterObjectData() {}

ColourFilter* ColourFilterObjectData::GetColourFilter() {
  if (!colour_filer_)
    colour_filer_.reset(graphics_system_.BuildColourFiller());
  return colour_filer_.get();
}

void ColourFilterObjectData::Render(const GraphicsObject& go,
                                    const GraphicsObject* parent) {
  auto& param = go.Param();

  if (param.width() != 100 || param.height() != 100) {
    static bool printed = false;
    if (!printed) {
      printed = true;
      std::cerr << "We can't yet scaling colour filters." << std::endl;
    }
  }

  RGBAColour colour = param.colour();
  GetColourFilter()->Fill(go, screen_rect_, colour);
}

int ColourFilterObjectData::PixelWidth(const GraphicsObject&) {
  throw rlvm::Exception("There is no sane value for this!");
}

int ColourFilterObjectData::PixelHeight(const GraphicsObject&) {
  throw rlvm::Exception("There is no sane value for this!");
}

std::unique_ptr<GraphicsObjectData> ColourFilterObjectData::Clone() const {
  return std::make_unique<ColourFilterObjectData>(graphics_system_,
                                                  screen_rect_);
}

void ColourFilterObjectData::Execute(RLMachine& machine) {
  // Nothing to do.
}

std::shared_ptr<const Surface> ColourFilterObjectData::CurrentSurface(
    const GraphicsObject&) {
  return std::shared_ptr<const Surface>();
}

ColourFilterObjectData::ColourFilterObjectData(System& system)
    : graphics_system_(system.graphics()) {}

template <class Archive>
void ColourFilterObjectData::serialize(Archive& ar, unsigned int version) {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this);
  ar & screen_rect_;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void ColourFilterObjectData::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
template void ColourFilterObjectData::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

BOOST_CLASS_EXPORT(ColourFilterObjectData);
