// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
// Copyright (C) 2025 Serina Sakurai
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

#include <chrono>
#include <memory>

class Clock;

// Frame Counters
//
// "Frame counters are designed to make it simple to ensure events happen at a
// constant speed regardless of the host system's specifications. Once a frame
// counter has been initialized, it will count from one arbitrary number to
// another, over a given length of time. The counter can be queried at any
// point to get its current value."

class FrameCounter {
 public:
  FrameCounter(std::shared_ptr<Clock> clock,
               int frame_min,
               int frame_max,
               int milliseconds);

  virtual ~FrameCounter();

  // Returns the current frame value
  virtual float ReadFrame() = 0;

  inline void SetFrame(int value) { value_ = static_cast<float>(value); }

  // Start or stop the timer
  void BeginTimer(
      std::chrono::milliseconds delay = std::chrono::milliseconds(0));

  // Terminate the frame counter
  // One-shot counters should yield the final value; Looping counters should
  // freeze the current value
  void EndTimer();

  inline bool IsFinished() const { return !is_active_; }
  inline bool IsActive() const {
    // Sometimes we call ReadFrame() internally to see if we've ended
    // but it's up to derived classes to end themselves or not.
    return is_active_;
  }
  inline void SetActive(bool active) { is_active_ = active; }

 protected:
  // Computes an un-clamped fraction of how far along we are, i.e.
  // 0.0 at start_time_, 1.0 at exactly total_time_, and >1.0 if time is beyond
  // total_time_. If total_time_ == 0 or is_active_ == false, it returns 1.0 by
  // default.
  double ComputeNormalizedTime() const;

  // Optionally used by derived classes to clamp fraction to [0..1] if they want
  // a one-shot timer.
  double ClampFractionToOneShot(double fraction);

  // Data members
  std::shared_ptr<Clock> clock_;

  float value_;
  int min_value_;
  int max_value_;
  bool is_active_;

  std::chrono::milliseconds start_time_;
  std::chrono::milliseconds total_time_;
};

class SimpleFrameCounter : public FrameCounter {
 public:
  SimpleFrameCounter(std::shared_ptr<Clock> clock,
                     int frame_min,
                     int frame_max,
                     int milliseconds)
      : FrameCounter(clock, frame_min, frame_max, milliseconds) {}

  virtual float ReadFrame() override;
};

class LoopFrameCounter : public FrameCounter {
 public:
  LoopFrameCounter(std::shared_ptr<Clock> clock,
                   int frame_min,
                   int frame_max,
                   int milliseconds)
      : FrameCounter(clock, frame_min, frame_max, milliseconds) {}

  virtual float ReadFrame() override;
};

class TurnFrameCounter : public FrameCounter {
 public:
  TurnFrameCounter(std::shared_ptr<Clock> clock,
                   int frame_min,
                   int frame_max,
                   int milliseconds)
      : FrameCounter(clock, frame_min, frame_max, milliseconds) {}

  virtual float ReadFrame() override;
};

class AcceleratingFrameCounter : public FrameCounter {
 public:
  AcceleratingFrameCounter(std::shared_ptr<Clock> clock,
                           int frame_min,
                           int frame_max,
                           int milliseconds)
      : FrameCounter(clock, frame_min, frame_max, milliseconds) {}

  virtual float ReadFrame() override;
};

class DeceleratingFrameCounter : public FrameCounter {
 public:
  DeceleratingFrameCounter(std::shared_ptr<Clock> clock,
                           int frame_min,
                           int frame_max,
                           int milliseconds)
      : FrameCounter(clock, frame_min, frame_max, milliseconds) {}

  virtual float ReadFrame() override;
};
