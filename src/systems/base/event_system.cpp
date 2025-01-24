// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#include "systems/base/event_system.hpp"

#include "core/frame_counter.hpp"
#include "core/gameexe.hpp"
#include "machine/long_operation.hpp"
#include "machine/rlmachine.hpp"
#include "systems/base/event_listener.hpp"
#include "utilities/exception.hpp"

#include <chrono>

// -----------------------------------------------------------------------
// EventSystem
// -----------------------------------------------------------------------
EventSystem::EventSystem()
    : shift_pressed_(false),
      ctrl_pressed_(false),
      mouse_inside_window_(true),
      mouse_pos_(),
      button1_state_(0),
      button2_state_(0),
      last_mouse_move_time_(0),

      clock_(std::make_shared<Clock>()) {}

EventSystem::~EventSystem() = default;

void EventSystem::AddListener(std::weak_ptr<EventListener> listener) {
  event_listeners_.insert(listener);
}

void EventSystem::RemoveListener(std::weak_ptr<EventListener> listener) {
  event_listeners_.erase(listener);
}

unsigned int EventSystem::GetTicks() const {
  return static_cast<unsigned int>(clock_->GetTicks().count());
}

std::chrono::time_point<std::chrono::steady_clock> EventSystem::GetTime()
    const {
  return clock_->GetTime();
}

std::shared_ptr<Clock> EventSystem::GetClock() const { return clock_; }

void EventSystem::DispatchEvent(
    const std::function<bool(EventListener&)>& event) {
  // In addition to the handled variable, we need to add break statements to
  // the loops since |event| can be any arbitrary code and may modify listeners
  // or handlers. (i.e., System::showSyscomMenu)
  bool handled = false;

  for (auto it = event_listeners_.begin();
       it != event_listeners_.end() && !handled;) {
    if (auto sp = it->lock()) {
      if (event(*sp)) {
        handled = true;
        break;
      }
      ++it;
    } else {
      it = event_listeners_.erase(it);
    }
  }
}

void EventSystem::BroadcastEvent(
    const std::function<void(EventListener&)>& event) {
  for (auto it = event_listeners_.begin(); it != event_listeners_.end();) {
    if (auto sp = it->lock()) {
      event(*sp);
      ++it;
    } else {
      it = event_listeners_.erase(it);
    }
  }
}
