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

#include <stdexcept>
#include <vector>

// -----------------------------------------------------------------------
// TextWindowButton
// -----------------------------------------------------------------------

BasicTextWindowButton::BasicTextWindowButton(std::shared_ptr<Clock> clock,
                                             bool enable,
                                             Rect button_rect)
    : state_(ButtonState::Normal), clock_(clock), btn_rect_(button_rect) {
  if (!enable || !IsValid())
    state_ = ButtonState::Unused;
  on_update_(*this);
}

bool BasicTextWindowButton::IsValid() const {
  return state_ != ButtonState::Unused && !btn_rect_.is_empty();
}

void BasicTextWindowButton::SetMousePosition(const Point& pos) {
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

bool BasicTextWindowButton::HandleMouseClick(RLMachine& machine,
                                             const Point& pos,
                                             bool pressed) {
  if (state_ == ButtonState::Disabled)
    return false;

  if (IsValid()) {
    bool in_box = btn_rect_.Contains(pos);

    if (in_box) {
      // Perform any activation
      if (pressed) {
        state_ = ButtonState::Pressed;
        ButtonPressed();
      } else {
        state_ = ButtonState::Highlighted;
        ButtonReleased(machine);
      }

      return true;
    }
  }

  return false;
}

void BasicTextWindowButton::Render(
    const std::shared_ptr<const Surface>& buttons,
    int base_pattern) {
  if (IsValid()) {
    GrpRect rect = buttons->GetPattern(base_pattern + static_cast<int>(state_));
    if (!rect.rect.is_empty()) {
      Rect dest = Rect(btn_rect_.origin(), rect.rect.size());
      buttons->RenderToScreen(rect.rect, dest, 255);
    }
  }
}

void BasicTextWindowButton::Execute() {
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
