// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#pragma once

#include "core/event.hpp"

#include <chrono>
#include <memory>
#include <set>

class IEventBackend;
class EventListener;
class Clock;

class EventSystem {
 public:
  explicit EventSystem(std::unique_ptr<IEventBackend>);
  virtual ~EventSystem();

  // rlvm event handling works by registering objects that received input
  // notifications from the EventSystem. These objects are EventListeners,
  // which passively listen for input and have a first chance grab at any click
  // or keypress.
  //
  // EventListeners with higher priority can grab the event before those having
  // lower priority. If a priority value is not given, it is set to default
  // value 0.
  void AddListener(int priority, std::weak_ptr<EventListener> listener);
  void AddListener(std::weak_ptr<EventListener> listener);
  void RemoveListener(std::weak_ptr<EventListener> listener);

  // Run once per cycle through the game loop to process events.
  void ExecuteEventSystem();

  // Returns the number of milliseconds since the program
  // started. Used for timing things.
  unsigned int GetTicks() const;
  std::chrono::time_point<std::chrono::steady_clock> GetTime() const;
  std::shared_ptr<Clock> GetClock() const;

 private:
  std::shared_ptr<Clock> clock_;

  std::unique_ptr<IEventBackend> event_backend_;

  // The container for event listeners.
  // This really should be a multi-index container, as we want to maintain it
  // using the pointer index and traverse it using the priority index. To
  // balance performance and simplicity, we implement a "lazy delete" mechanism
  // by throwing whatever we want to remove into a set, and defer the actual
  // deletion to traverse. This works because traversals are often, and the
  // client code is not likely going to repeatedly insert/remove the same
  // listener.
  struct Compare {
    bool operator()(
        const std::pair<int, std::weak_ptr<EventListener>>& lhs,
        const std::pair<int, std::weak_ptr<EventListener>>& rhs) const {
      // Compare the int values first
      if (lhs.first != rhs.first) {
        return lhs.first > rhs.first;
      }
      // Compare the weak_ptrs
      return lhs.second.owner_before(rhs.second);
    }
  };
  std::set<std::pair<int, std::weak_ptr<EventListener>>, Compare>
      event_listeners_;
  std::set<std::weak_ptr<EventListener>,
           std::owner_less<std::weak_ptr<EventListener>>>
      lazy_deleted_;
};
