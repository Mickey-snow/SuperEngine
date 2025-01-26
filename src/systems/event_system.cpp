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

#include "systems/event_system.hpp"

#include "core/event_listener.hpp"
#include "systems/event_backend.hpp"

// -----------------------------------------------------------------------
// EventSystem
// -----------------------------------------------------------------------
EventSystem::EventSystem(std::unique_ptr<IEventBackend> backend)
    : clock_(std::make_shared<Clock>()), event_backend_(std::move(backend)) {}

EventSystem::~EventSystem() = default;

void EventSystem::AddListener(int priority,
                              std::weak_ptr<EventListener> listener) {
  event_listeners_.insert(std::make_pair(priority, listener));
}
void EventSystem::AddListener(std::weak_ptr<EventListener> listener) {
  constexpr auto default_priority = 0;
  AddListener(default_priority, listener);
}
void EventSystem::RemoveListener(std::weak_ptr<EventListener> listener) {
  lazy_deleted_.insert(listener);
}

unsigned int EventSystem::GetTicks() const {
  return static_cast<unsigned int>(clock_->GetTicks().count());
}

std::chrono::time_point<std::chrono::steady_clock> EventSystem::GetTime()
    const {
  return clock_->GetTime();
}

std::shared_ptr<Clock> EventSystem::GetClock() const { return clock_; }

void EventSystem::ExecuteEventSystem() {
  while (true) {
    std::shared_ptr<Event> event = event_backend_->PollEvent();
    if (event == nullptr)
      return;

    for (auto it = event_listeners_.begin(); it != event_listeners_.end();) {
      if (std::holds_alternative<std::monostate>(*event))
        return;

      auto wp = it->second;

      if (lazy_deleted_.contains(wp)) {
        lazy_deleted_.erase(wp);
        it = event_listeners_.erase(it);
        continue;
      }

      if (auto sp = wp.lock()) {
        sp->OnEvent(event);
        ++it;
      } else {
        it = event_listeners_.erase(it);
      }
    }

    // when we finished the whole loop
    lazy_deleted_.clear();
  }
}
