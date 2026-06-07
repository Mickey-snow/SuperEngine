// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2008 Elliot Glaysher
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

#pragma once

#include "core/rect.hpp"
#include "utilities/clock.hpp"

#include <chrono>
#include <memory>

class SDLSurface;

// Represents a mouse cursor on screen.
class MouseCursor {
 public:
  explicit MouseCursor(std::shared_ptr<Clock> clock,
                       const std::shared_ptr<const SDLSurface>& cursor_surface,
                       int count,
                       int speed);
  ~MouseCursor();

  // Updates the MouseCursor.
  void Execute();

  // Renders the cursor to the screen, taking the hotspot offset into account.
  void RenderHotspotAt(const Point& mouse_pt);

 private:
  // Returns (renderX, renderY) which is the upper left corner of where the
  // cursor is to be rendered for the incoming mouse location (mouseX, mouseY).
  Point GetTopLeftForHotspotAt(const Point& mouse_location);

  // Sets hotspot_[XY] to the white pixel in the
  void FindHotspot();

  // The raw image read from the PDT.
  std::shared_ptr<const SDLSurface> cursor_surface_;

  // The number of frames in cursor.
  int count_;

  // How much time should be spent between mouse cursor frames.
  std::chrono::milliseconds frame_speed_;

  // The current frame.
  int current_frame_;

  // The last time current_frame_ was incremented
  Clock::timepoint_t last_time_frame_incremented_;

  // The hotspot location.
  Size hotspot_offset_;

  // The clock from event system
  std::shared_ptr<Clock> clock_;
};  // end of class MouseCursor
