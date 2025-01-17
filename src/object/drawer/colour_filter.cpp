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

#include "core/colour.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/gl_frame_buffer.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"
#include "systems/sdl_surface.hpp"

ColourFilterObjectData::ColourFilterObjectData(const Rect& screen_rect)
    : screen_rect_(screen_rect) {}

ColourFilterObjectData::~ColourFilterObjectData() {}

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

  auto screen_canvas = SDLSurface::screen_;
  auto background = screen_canvas->GetTexture();

  const Rect src(Point(0, 0), background->GetSize());
  const Rect dst(Point(0, 0), screen_canvas->GetSize());
  glRenderer().Render({background, src}, go.CreateRenderingConfig(),
                      {screen_canvas, dst});
}

int ColourFilterObjectData::PixelWidth(const GraphicsObject&) {
  throw std::runtime_error("There is no sane value for this!");
}

int ColourFilterObjectData::PixelHeight(const GraphicsObject&) {
  throw std::runtime_error("There is no sane value for this!");
}

std::unique_ptr<GraphicsObjectData> ColourFilterObjectData::Clone() const {
  return std::make_unique<ColourFilterObjectData>(screen_rect_);
}

void ColourFilterObjectData::Execute(RLMachine& machine) {
  // Nothing to do.
}

std::shared_ptr<const Surface> ColourFilterObjectData::CurrentSurface(
    const GraphicsObject&) {
  return std::shared_ptr<const Surface>();
}
