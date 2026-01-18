// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#include "core/rect.hpp"
#include "utilities/clock.hpp"

#include <functional>
#include <memory>
#include <optional>

class RLMachine;
class SDLSurface;
using Surface = SDLSurface;

// Describes the state of a Waku button
enum class ButtonState : int {
  Unused = -1,
  Normal = 0,
  Highlighted = 1,
  Pressed = 2,
  Activated = 4,
  Disabled = 3
};

// The base class for text window buttons
class BasicTextWindowButton {
 public:
  explicit BasicTextWindowButton(std::shared_ptr<Clock> clock,
                                 bool enable,
                                 Rect button_rect);

  // Checks to see if this is a valid, used button
  bool IsValid() const;

  // Track the mouse position to see if we need to alter our state
  void SetMousePosition(const Point& pos);

  bool HandleMouseClick(RLMachine& machine, const Point& pos, bool pressed);

  void Render(const std::shared_ptr<const Surface>& buttons, int base_pattern);

  // Called by other execute() calls while the System object has its
  // turn to do any updating
  void Execute();

  // Called before Execute() call, handles periodical updates.
  std::function<void(BasicTextWindowButton&)> on_update_;

  // Called when the button is pressed
  std::function<void()> on_pressed_;

  // Called when the button is released
  std::function<void()> on_release_;

  // When set, repeat the pressed callback periodically
  std::optional<std::chrono::milliseconds> time_between_invocations_;

  ButtonState state_;

 private:
  inline void ButtonPressed() {
    if (on_pressed_) {
      on_pressed_();
      last_invocation_ = clock_->GetTime();
    }
  }

  inline void ButtonReleased(RLMachine&) {
    if (on_release_)
      on_release_();
    last_invocation_.reset();
  }

  std::shared_ptr<Clock> clock_;
  std::optional<Clock::timepoint_t> last_invocation_;

  Rect btn_rect_;
};
