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

#include "systems/base/text_window_button.hpp"

#include "core/rect.hpp"
#include "systems/base/text_window.hpp"
#include "systems/sdl_surface.hpp"

// -----------------------------------------------------------------------
// TextWindowButton
// -----------------------------------------------------------------------

TextWindowButton::TextWindowButton(std::shared_ptr<Clock> clock,
                                   bool should_use,
                                   Rect button_rect)
    : state_(ButtonState::Normal),
      clock_(clock),
      btn_rect_(button_rect),
      button_surface_(nullptr),
      base_pattern_(0) {
  if (!should_use || !IsValid())
    state_ = ButtonState::Unused;
}

bool TextWindowButton::IsValid() const {
  return state_ != ButtonState::Unused && !btn_rect_.is_empty();
}

void TextWindowButton::SetMousePosition(const Point& pos) {
  if (state_ == ButtonState::Disabled)
    return;

  if (IsValid()) {
    bool in_box = btn_rect_.Contains(pos);
    if (in_box && state_ == ButtonState::Normal)
      state_ = ButtonState::Highlighted;
    else if (!in_box && state_ == ButtonState::Highlighted)
      state_ = ButtonState::Normal;
    else if (!in_box && state_ == ButtonState::Pressed)
      state_ = ButtonState::Normal;
  }
}

bool TextWindowButton::HandleMouseClick(const Point& pos, bool pressed) {
  if (state_ == ButtonState::Disabled)
    return false;

  if (!pressed && state_ == ButtonState::Pressed) {
    ButtonReleased();
    state_ = btn_rect_.Contains(pos) ? ButtonState::Highlighted
                                     : ButtonState::Normal;
    return true;
  }

  if (IsValid()) {
    bool in_box = btn_rect_.Contains(pos);

    if (in_box) {
      // Perform any activation
      if (pressed) {
        state_ = ButtonState::Pressed;
        ButtonPressed();
      } else {
        state_ = ButtonState::Highlighted;
        ButtonReleased();
      }

      return true;
    }
  }

  return false;
}

void TextWindowButton::SetSurface(std::shared_ptr<Surface> surf,
                                  int base_pattern) {
  button_surface_ = surf;
  base_pattern_ = base_pattern;
}

std::pair<std::shared_ptr<Surface>, Rect> TextWindowButton::Render() const {
  if (!IsValid() || !button_surface_)
    return std::make_pair(nullptr, Rect());
  int offset = base_pattern_ + static_cast<int>(state_);
  GrpRect src = button_surface_->GetPattern(offset);
  return std::make_pair(button_surface_, src.rect);
}

void TextWindowButton::Execute() {
  if (on_update_)
    on_update_(*this);

  if (!last_invocation_ || !time_between_invocations_)
    return;

  auto cur_time = clock_->GetTime();
  if (*last_invocation_ + *time_between_invocations_ <= cur_time)
    if (on_pressed_) {
      on_pressed_();
      last_invocation_ = cur_time;
    }
}
