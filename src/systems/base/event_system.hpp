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

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <set>

#include "core/rect.hpp"
#include "utilities/clock.hpp"

class RLMachine;
class Gameexe;
class FrameCounter;
class EventListener;

// -----------------------------------------------------------------------

// Generalization of an event system. Reallive's event model is a bit
// weird; interpreted code will check the state of certain keyboard
// modifiers, with functions such as CtrlPressed() or ShiftPressed().
//
// So what's the solution? Have two different event systems side by
// side. One is exposed to Reallive and mimics what RealLive bytecode
// expects. The other is based on event handlers and is sane.
class EventSystem {
 public:
  explicit EventSystem();
  virtual ~EventSystem();

  // Frame Counters
  //
  // "Frame counters are designed to make it simple to ensure events happen at a
  // constant speed regardless of the host system's specifications. Once a frame
  // counter has been initialized, it will count from one arbitrary number to
  // another, over a given length of time. The counter can be queried at any
  // point to get its current value."
  //
  // Valid values for layer are 0 and 1. Valid values for frame_counter are 0
  // through 255.
  void SetFrameCounter(int layer, int frame_counter, FrameCounter* counter);
  FrameCounter& GetFrameCounter(int layer, int frame_counter);
  bool FrameCounterExists(int layer, int frame_counter);

  // Keyboard and Mouse Input (Event Listener style)
  //
  // rlvm event handling works by registering objects that received input
  // notifications from the EventSystem. These objects are EventListeners,
  // which passively listen for input and have a first chance grab at any click
  // or keypress.
  //
  // If no EventListener claims the event, then we try to reinterpret the top
  // of the RLMachine callstack as an EventListener. Otherwise, the events
  // are handled RealLive style (see below).
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

  // Returns whether shift is currently pressed.
  virtual bool ShiftPressed() const = 0;

  // Returns whether ctrl has been pressed since the last invocation of
  // ctrlPresesd().
  virtual bool CtrlPressed() const = 0;

  // Returns the current cursor hotspot.
  virtual Point GetCursorPos() = 0;

  // Gets the location of the mouse cursor and the button states.
  //
  // The following values are used to indicate a button's status:
  // - 0 if unpressed
  // - 1 if being pressed
  // - 2 if pressed and released.
  virtual void GetCursorPos(Point& position, int& button1, int& button2) = 0;

  // Resets the state of the mouse buttons.
  virtual void FlushMouseClicks() = 0;

  // Returns the time in ticks of the last mouse movement.
  virtual unsigned int TimeOfLastMouseMove() = 0;

 protected:
  // Calls a EventListener member function on all event listeners, and then
  // event handlers, stopping when an object says they handled it.
  void DispatchEvent(const std::function<bool(EventListener&)>& event);
  void BroadcastEvent(const std::function<void(EventListener&)>& event);

 private:
  // Helper function that verifies input
  void CheckLayerAndCounter(int layer, int counter);

  std::unique_ptr<FrameCounter> frame_counters_[2][255];

  std::shared_ptr<Clock> clock_;

  std::set<std::weak_ptr<EventListener>,
           std::owner_less<std::weak_ptr<EventListener>>>
      event_listeners_;
};
