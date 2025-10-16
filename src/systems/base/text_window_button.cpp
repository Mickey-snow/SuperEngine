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

#include "systems/base/text_window_button.hpp"

#include <stdexcept>
#include <vector>

#include "core/notification/details.hpp"
#include "core/notification/service.hpp"
#include "core/rect.hpp"
#include "machine/long_operation.hpp"
#include "machine/rlmachine.hpp"
#include "modules/jump.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/system_error.hpp"
#include "systems/base/text_system.hpp"
#include "systems/base/text_window.hpp"
#include "systems/event_system.hpp"
#include "systems/sdl_surface.hpp"

// -----------------------------------------------------------------------
// TextWindowButton
// -----------------------------------------------------------------------

BasicTextWindowButton::BasicTextWindowButton(std::shared_ptr<Clock> clock,
                                             bool enable,
                                             Rect button_rect)
    : clock_(clock), state_(ButtonState::Normal), btn_rect_(button_rect) {
  if (!enable || !IsValid())
    state_ = ButtonState::Unused;
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

// -----------------------------------------------------------------------
// ActivationTextWindowButton
// -----------------------------------------------------------------------

ActivationTextWindowButton::ActivationTextWindowButton(
    std::shared_ptr<Clock> clock,
    bool use,
    Rect btn_rect,
    CallbackFunction setter)
    : BasicTextWindowButton(clock, use, btn_rect),
      on_set_(setter),
      on_(false),
      enabled_(true),
      enabled_listener_(static_cast<NotificationType::Type>(-1)),
      change_listener_(static_cast<NotificationType::Type>(-1)) {
  on_release_ = [this]() {
    if (enabled_) {
      on_set_(!on_);
    }
  };
}

void ActivationTextWindowButton::SetEnabled(bool enabled) {
  enabled_ = enabled;
  SetState();
}

void ActivationTextWindowButton::SetEnabledNotification(
    NotificationType enabled_listener) {
  enabled_listener_ = enabled_listener;
  registrar_.Add(this, enabled_listener_, NotificationService::AllSources());
}

void ActivationTextWindowButton::SetChangeNotification(
    NotificationType change_listener) {
  change_listener_ = change_listener;
  registrar_.Add(this, change_listener_, NotificationService::AllSources());
}

void ActivationTextWindowButton::SetState() {
  if (enabled_)
    state_ = on_ ? ButtonState::Activated : ButtonState::Normal;
  else
    state_ = ButtonState::Disabled;
}

void ActivationTextWindowButton::Observe(NotificationType type,
                                         const NotificationSource&,
                                         const NotificationDetails& details) {
  if (type == enabled_listener_) {
    Details<int> i(details);
    enabled_ = *i.ptr();
  } else if (type == change_listener_) {
    Details<int> i(details);
    on_ = *i.ptr();
  }

  SetState();
}

// -----------------------------------------------------------------------
// RepeatActionWhileHoldingWindowButton
// -----------------------------------------------------------------------

RepeatActionWhileHoldingWindowButton::RepeatActionWhileHoldingWindowButton(
    std::shared_ptr<Clock> clock,
    bool use,
    Rect btn_rect,
    CallbackFunction callback,
    std::chrono::milliseconds time_between_invocations)
    : BasicTextWindowButton(clock, use, btn_rect),
      callback_(callback),
      held_down_(false),
      time_between_invocations_(time_between_invocations) {
  on_pressed_ = [this]() {
    held_down_ = true;
    callback_();
    last_invocation_ = clock_->GetTime();
  };
  on_release_ = [this]() { held_down_ = false; };
}

void RepeatActionWhileHoldingWindowButton::Execute() {
  if (held_down_) {
    auto cur_time = clock_->GetTime();

    if (last_invocation_ + time_between_invocations_ <= cur_time) {
      callback_();
      last_invocation_ = cur_time;
    }
  }
}

// -----------------------------------------------------------------------
// ExbtnWindowButton
// -----------------------------------------------------------------------

ExbtnWindowButton::ExbtnWindowButton(std::shared_ptr<Clock> clock,
                                     bool use,
                                     Rect btn_rect,
                                     int to_scenario,
                                     int to_entrypoint)
    : BasicTextWindowButton(clock, use, btn_rect),
      scenario_(to_scenario),
      entrypoint_(to_entrypoint) {
  if (!use || !IsValid())
    scenario_ = entrypoint_ = 0;
}

void ExbtnWindowButton::ButtonReleased(RLMachine& machine) {
  // Hide all text boxes when entering an Exbtn
  machine.GetSystem().text().set_system_visible(false);

  // Push a LongOperation onto the stack which will restore
  // visibility when we return from this Exbtn call
  machine.PushLongOperation(std::make_shared<RestoreTextSystemVisibility>());
  Farcall(machine, scenario_, entrypoint_);
}
