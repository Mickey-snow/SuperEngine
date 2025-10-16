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

#include "core/notification/observer.hpp"
#include "core/notification/registrar.hpp"
#include "core/notification/type.hpp"
#include "core/rect.hpp"
#include "utilities/clock.hpp"

#include <functional>
#include <memory>
#include <vector>

class Point;
class RLMachine;
class Rect;
class SDLSurface;
using Surface = SDLSurface;
class System;
class TextWindow;

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
  virtual ~BasicTextWindowButton() = default;

  // Checks to see if this is a valid, used button
  bool IsValid() const;

  // Track the mouse position to see if we need to alter our state
  void SetMousePosition(const Point& pos);

  bool HandleMouseClick(RLMachine& machine, const Point& pos, bool pressed);

  void Render(const std::shared_ptr<const Surface>& buttons, int base_pattern);

  // Called by other execute() calls while the System object has its
  // turn to do any updating
  virtual void Execute() {}

  // Called when the button is pressed
  std::function<void()> on_pressed_;

  // Called when the button is released
  std::function<void()> on_release_;

 protected:
  virtual void ButtonPressed() {
    if (on_pressed_)
      on_pressed_();
  }

  virtual void ButtonReleased(RLMachine&) {
    if (on_release_)
      on_release_();
  }

 protected:
  std::shared_ptr<Clock> clock_;

  ButtonState state_;
  Rect btn_rect_;
};

// -----------------------------------------------------------------------

class ActivationTextWindowButton : public BasicTextWindowButton,
                                   public NotificationObserver {
 public:
  typedef std::function<void(int)> CallbackFunction;

 public:
  ActivationTextWindowButton(std::shared_ptr<Clock> clock,
                             bool use,
                             Rect btn_rect,
                             CallbackFunction setter);

  void SetEnabled(bool enabled);

  void SetEnabledNotification(NotificationType enabled_listener);
  void SetChangeNotification(NotificationType change_listener);

 private:
  void SetState();

  // NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) override;

  CallbackFunction on_set_;
  bool on_;
  bool enabled_;

  // The type that we listen for to change our activated state.
  NotificationType enabled_listener_;
  NotificationType change_listener_;
  NotificationRegistrar registrar_;
};

// -----------------------------------------------------------------------

class RepeatActionWhileHoldingWindowButton : public BasicTextWindowButton {
 public:
  using CallbackFunction = std::function<void(void)>;

  RepeatActionWhileHoldingWindowButton(
      std::shared_ptr<Clock> clock,
      bool use,
      Rect btn_rect,
      CallbackFunction callback,
      std::chrono::milliseconds time_between_invocations);

  virtual void Execute() override;

 private:
  CallbackFunction callback_;
  bool held_down_;
  Clock::timepoint_t last_invocation_;
  std::chrono::milliseconds time_between_invocations_;
};

// -----------------------------------------------------------------------

class ExbtnWindowButton : public BasicTextWindowButton {
 public:
  ExbtnWindowButton(std::shared_ptr<Clock> clock,
                    bool use,
                    Rect btn_rect,
                    int to_scenario,
                    int to_entrypoint);

  virtual void ButtonReleased(RLMachine& machine) override;

 private:
  int scenario_;
  int entrypoint_;
};
