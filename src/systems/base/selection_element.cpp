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

#include "systems/base/selection_element.hpp"

#include "core/rect.hpp"
#include "systems/sdl_surface.hpp"

#include <utility>

// -----------------------------------------------------------------------
// SelectionElement
// -----------------------------------------------------------------------
SelectionElement::SelectionElement(std::shared_ptr<Surface> normal_image,
                                   std::shared_ptr<Surface> highlighted_image,
                                   Point pos)
    : is_highlighted_(false),
      upper_right_(pos),
      normal_image_(normal_image),
      highlighted_image_(highlighted_image) {}

bool SelectionElement::IsHighlightedAt(const Point& p) {
  return Rect(upper_right_, normal_image_->GetSize()).Contains(p);
}

void SelectionElement::SetMousePosition(const Point& pos) {
  if (!std::exchange(is_highlighted_, IsHighlightedAt(pos)) &&
      is_highlighted_) {
    if (on_mouseover_)
      on_mouseover_();
  }
}

bool SelectionElement::HandleMouseClick(const Point& pos, bool pressed) {
  if (pressed == false && IsHighlightedAt(pos)) {
    // Released within the button
    if (selection_callback_)
      selection_callback_();

    return true;
  } else {
    return false;
  }
}

void SelectionElement::Render() {
  std::shared_ptr<Surface> target;

  if (is_highlighted_)
    target = highlighted_image_;
  else
    target = normal_image_;

  Size s = target->GetSize();

  target->RenderToScreen(Rect(Point(0, 0), s), Rect(upper_right_, s), 255);
}
