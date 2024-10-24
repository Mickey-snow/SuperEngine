// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#ifndef SRC_UTILITIES_STOPWATCH_H_
#define SRC_UTILITIES_STOPWATCH_H_

#include <chrono>
#include <stdexcept>

class Stopwatch {
 public:
  using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;
  using duration_t = std::chrono::milliseconds;

 public:
  Stopwatch() : state_(State::Paused), time_(duration_t::zero()) {}

  enum class Action { Pause, Run, Reset };
  virtual void Apply(Action action, timepoint_t now) {
    Notify(now);

    switch (action) {
      case Action::Pause: {
        if (state_ != State::Set)
          state_ = State::Paused;
      } break;

      case Action::Run:
        state_ = State::Running;
        break;

      case Action::Reset: {
        time_ = decltype(time_)::zero();
        state_ = State::Set;
      } break;

      default:
        throw std::invalid_argument("Stopwatch: invalid action " +
                                    std::to_string(static_cast<int>(action)));
    }
  }

  enum class State { Paused, Running, Set };
  State GetState(void) const { return state_; }

  duration_t GetReading() const { return time_; }

  void Notify(timepoint_t tp) {
    if (state_ == State::Running)
      time_ +=
          std::chrono::duration_cast<duration_t>(tp - last_tick_);
    last_tick_ = tp;
  }

 private:
  State state_;
  timepoint_t last_tick_;
  duration_t time_;
};

#endif
