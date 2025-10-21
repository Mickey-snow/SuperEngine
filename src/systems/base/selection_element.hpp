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

#pragma once

#include <functional>
#include <memory>

#include "core/rect.hpp"

class SDLSurface;
using Surface = SDLSurface;

// Represents a clickable element inside TextWindows.
class SelectionElement {
 public:
  SelectionElement(std::shared_ptr<Surface> normal_image,
                   std::shared_ptr<Surface> highlighted_image,
                   Point pos);

  void SetMousePosition(const Point& pos);
  bool HandleMouseClick(const Point& pos, bool pressed);

  void Render();

  inline void OnMouseover(std::function<void()> mouseover_callback) {
    on_mouseover_ = std::move(mouseover_callback);
  }

  inline void OnSelect(std::function<void()> selection_callback) {
    selection_callback_ = std::move(selection_callback);
  }

 private:
  bool IsHighlightedAt(const Point& p);

  bool is_highlighted_;

  Point upper_right_;

  std::shared_ptr<Surface> normal_image_;
  std::shared_ptr<Surface> highlighted_image_;

  // Callback function for when item is selected.
  std::function<void()> selection_callback_;

  // Callback function when cursor is over the selection item
  std::function<void()> on_mouseover_;
};
