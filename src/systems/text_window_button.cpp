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

#include "systems/text_window_button.hpp"

#include "core/rect.hpp"
#include "systems/sdl/sdl_surface.hpp"
#include "systems/text_window.hpp"

// -----------------------------------------------------------------------
// TextWindowButton
// -----------------------------------------------------------------------

TextWindowButton::TextWindowButton(std::shared_ptr<Clock> clock,
                                   bool should_use,
                                   Rect button_rect)
    : state_(TextWindowButtonState::Normal),
      clock_(clock),
      btn_rect_(button_rect),
      button_surface_(nullptr),
      base_pattern_(0) {
  if (!should_use || !IsValid())
    state_ = TextWindowButtonState::Unused;
}

bool TextWindowButton::IsValid() const {
  return state_ != TextWindowButtonState::Unused && !btn_rect_.is_empty();
}

void TextWindowButton::SetMousePosition(const Point& pos) {
  if (state_ == TextWindowButtonState::Disabled)
    return;

  if (IsValid()) {
    bool in_box = btn_rect_.Contains(pos);
    if (in_box && state_ == TextWindowButtonState::Normal)
      state_ = TextWindowButtonState::Highlighted;
    else if (!in_box && state_ == TextWindowButtonState::Highlighted)
      state_ = TextWindowButtonState::Normal;
    else if (!in_box && state_ == TextWindowButtonState::Pressed)
      state_ = TextWindowButtonState::Normal;
  }
}

bool TextWindowButton::HandleMouseClick(const Point& pos, bool pressed) {
  if (state_ == TextWindowButtonState::Disabled)
    return false;

  if (!pressed && state_ == TextWindowButtonState::Pressed) {
    ButtonReleased();
    state_ = btn_rect_.Contains(pos) ? TextWindowButtonState::Highlighted
                                     : TextWindowButtonState::Normal;
    return true;
  }

  if (IsValid()) {
    bool in_box = btn_rect_.Contains(pos);

    if (in_box) {
      // Perform any activation
      if (pressed) {
        state_ = TextWindowButtonState::Pressed;
        ButtonPressed();
      } else {
        state_ = TextWindowButtonState::Highlighted;
        ButtonReleased();
      }

      return true;
    }
  }

  return false;
}

void TextWindowButton::SetSurface(std::shared_ptr<SDLSurface> surf,
                                  int base_pattern) {
  button_surface_ = surf;
  base_pattern_ = base_pattern;
}

void TextWindowButton::SetActionTableEntry(ButtonActionTable::Entry entry,
                                           int cut_no) {
  action_entry_ = std::move(entry);
  base_pattern_ = cut_no;
}

namespace {
ButtonState ToButtonState(TextWindowButtonState state) {
  switch (state) {
    case TextWindowButtonState::Highlighted:
      return ButtonState::Hit;
    case TextWindowButtonState::Pressed:
      return ButtonState::Push;
    case TextWindowButtonState::Activated:
      return ButtonState::Select;
    case TextWindowButtonState::Disabled:
    case TextWindowButtonState::Unused:
      return ButtonState::Disable;
    case TextWindowButtonState::Normal:
    default:
      return ButtonState::Normal;
  }
}
}  // namespace

std::pair<std::shared_ptr<SDLSurface>, Rect> TextWindowButton::Render() const {
  if (!IsValid() || !button_surface_)
    return std::make_pair(nullptr, Rect());
  int offset = base_pattern_ + static_cast<int>(state_);
  if (action_entry_)
    offset =
        base_pattern_ + action_entry_->GetState(ToButtonState(state_)).pattern;
  GrpRect src = button_surface_->GetPattern(offset);
  return std::make_pair(button_surface_, src.rect);
}

Point TextWindowButton::GetRenderOffset() const {
  if (!action_entry_)
    return {};
  return action_entry_->GetState(ToButtonState(state_)).rep_pos;
}

int TextWindowButton::GetRenderAlpha() const {
  if (!action_entry_)
    return 255;
  return action_entry_->GetState(ToButtonState(state_)).rep_tr;
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
