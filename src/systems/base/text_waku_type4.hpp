// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#pragma once

#include "systems/base/text_waku.hpp"
#include "systems/sdl_surface.hpp"

// Waku which is a modified Ninebox. Instead of a ninebox, it's really a 12-box
// where four of the entries aren't used and the center is never defined. This
// box expands to fill whatever size is passed into render().
class TextWakuType4 : public TextWaku {
 public:
  TextWakuType4();

  virtual void Execute() override;
  virtual void Render(Point box_location,
                      Size namebox_size,
                      RGBAColour colour,
                      bool is_filter) override;

  // We have no size other than what is passed to |namebox_size|. Always
  // returns false and resets |out|.
  virtual Size GetSize(const Size& text_surface) const override;

  // TODO(erg): These two methods shouldn't really exist; I need to redo
  // plumbing of events so that these aren't routed through TextWindow, but are
  // instead some sort of listener. I'm currently thinking that the individual
  // buttons that need to handle events should be listeners.
  // normal waku only
  virtual void SetMousePosition(const Point& pos) override {}
  virtual bool HandleMouseClick(const Point& pos, bool pressed) override {
    return false;
  }

  void SetMainWaku(std::shared_ptr<const Surface> waku_surface);
  void SetArea(int top, int bottom, int left, int right);

 private:
  // Returns |cached_backing_|, shrinking or enlarging it to |size|.
  const std::shared_ptr<Surface>& GetWakuBackingOfSize(Size size);

  // Additional area that adds to the filter backing in four directions.
  int area_top_, area_bottom_, area_left_, area_right_;

  // The surface that we pick pieces of our textbox against.
  std::shared_ptr<const Surface> waku_main_;

  // A cached backing regenerated whenever the namebox size changes
  std::shared_ptr<Surface> cached_backing_;

  // G00 regions in |waku_main_|.
  GrpRect top_left_, top_center_, top_right_;
  GrpRect left_side_, right_side_;
  GrpRect bottom_left_, bottom_center_, bottom_right_;
};
