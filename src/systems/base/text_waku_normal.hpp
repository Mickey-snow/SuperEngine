// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#include <memory>
#include <string>
#include <vector>

class Gameexe;
class Point;
class Rect;
class RLMachine;
class Size;
class SDLSurface;
using Surface = SDLSurface;
class System;
class TextWindowButton;

// Container class that owns all text window decorations.
//
// Window decorations are defined with \#WAKU.<setno>.<no>. Gameexe.ini keys.
class TextWakuNormal : public TextWaku {
 public:
  TextWakuNormal(int setno, int no);

  void AddButton(std::string btn_name,
                 std::unique_ptr<TextWindowButton> btn_impl);

  virtual void Execute() override;
  virtual void Render(Point box_location,
                      Size namebox_size,
                      RGBAColour colour,
                      bool is_filter) override;
  virtual Size GetSize(const Size& text_surface) const override;
  virtual Point InsertionPoint(const Rect& waku_rect,
                               const Size& padding,
                               const Size& surface_size,
                               bool center) const override;

  // TODO(erg): These two methods shouldn't really exist; I need to redo
  // plumbing of events so that these aren't routed through TextWindow, but are
  // instead some sort of listener. I'm currently thinking that the individual
  // buttons that need to handle events should be listeners.
  virtual void SetMousePosition(const Point& pos) override;
  virtual bool HandleMouseClick(const Point& pos, bool pressed) override;

  void SetWakuMain(std::shared_ptr<const Surface> surface);
  void SetWakuBacking(std::shared_ptr<const Surface> surface);

 private:
  int setno_, no_;

  std::shared_ptr<const Surface> main_surface_;
  std::shared_ptr<Surface> backing_surface_;

  struct WakuButton {
    std::string name;
    std::unique_ptr<TextWindowButton> btn;
  };
  std::vector<WakuButton> buttons_;
};
