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
#include "core/rect.hpp"
#include "utilities/clock.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <set>

class RLMachine;
class Gameexe;
class FrameCounter;
class EventListener;

// Generalization of an event system. Reallive's event model is a bit
// weird; interpreted code will check the state of certain keyboard
// modifiers, with functions such as CtrlPressed() or ShiftPressed().
class EventSystem {
 public:
  explicit EventSystem();
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
  virtual void ExecuteEventSystem(RLMachine& machine) = 0;

  // Returns the number of milliseconds since the program
  // started. Used for timing things.
  virtual unsigned int GetTicks() const;
  virtual std::chrono::time_point<std::chrono::steady_clock> GetTime() const;
  virtual std::shared_ptr<Clock> GetClock() const;

  // Keyboard and Mouse Input (Reallive style)
  //
  // RealLive applications poll for input, with all the problems that sort of
  // event handling has. We therefore provide an interface for polling.
  //
  // Don't use it. This interface is provided for RealLive
  // bytecode. EventListeners should be used within rlvm code, instead.
  bool mouse_inside_window() const { return mouse_inside_window_; }

  // Returns whether shift is currently pressed.
  bool ShiftPressed() const { return shift_pressed_; }

  // Returns whether ctrl has been pressed since the last invocation of
  // ctrlPresesd().
  bool CtrlPressed() const { return ctrl_pressed_; }

  // Returns the current cursor hotspot.
  Point GetCursorPos() const { return mouse_pos_; }

  // Gets the location of the mouse cursor and the button states.
  //
  // The following values are used to indicate a button's status:
  // - 0 if unpressed
  // - 1 if being pressed
  // - 2 if pressed and released.
  void GetCursorPos(Point& position, int& button1, int& button2) {
    position = mouse_pos_;
    button1 = button1_state_;
    button2 = button2_state_;
  }

  // Resets the state of the mouse buttons.
  void FlushMouseClicks() {
    button1_state_ = 0;
    button2_state_ = 0;
  }

  // Returns the time in ticks of the last mouse movement.
  unsigned int TimeOfLastMouseMove() { return last_mouse_move_time_; }

 protected:
  void DispatchEvent(std::shared_ptr<Event> event);

  bool shift_pressed_, ctrl_pressed_;

  // Whether the mouse cursor is currently inside the window bounds.
  bool mouse_inside_window_;

  Point mouse_pos_;

  int button1_state_, button2_state_;

  // The last time we received a mouse move notification.
  unsigned int last_mouse_move_time_;

 private:
  std::shared_ptr<Clock> clock_;

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
