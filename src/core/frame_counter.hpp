// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
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

#include <memory>

class Clock;

class FrameCounter {
 public:
  FrameCounter(std::shared_ptr<Clock> clock,
               int frame_min,
               int frame_max,
               int milliseconds);

  virtual ~FrameCounter();

  // Returns the current value of this frame counter, a value between
  virtual int ReadFrame() = 0;

  // The converse is setting the value, which should be done after the
  // frame counter has been turned off.
  void set_value(int value) { value_ = value; }

  // When a timer starts, we need to tell the EventSystem that we now
  // have a near realtime event going on and to stop being nice to the
  // operating system.
  void BeginTimer();

  // When a timer ends, there's no need to be so harsh on the
  // system. Tell the event_system that we no longer require near
  // realtime event handling.
  void EndTimer();

  bool IsActive();
  void set_active(bool active) { is_active_ = active; }

  bool CheckIfFinished(float new_value);
  void UpdateTimeValue(float num_ticks);

 protected:
  // Implementation of ReadFrame(). Called by most subclasses with their own
  // data members.
  int ReadNormalFrameWithChangeInterval(float change_interval,
                                        float& time_at_last_check);

  // Called from read_normal_frame_with_change_interval when finished. This
  // method can be overloaded to control what happens when the timer
  // has reached its end.
  virtual void Finished();

  std::shared_ptr<Clock> clock_;

  float value_;
  int min_value_;
  int max_value_;
  bool is_active_;

  unsigned int time_at_start_;
  unsigned int total_time_;
};

// Simple frame counter that counts from frame_min to frame_max.
class SimpleFrameCounter : public FrameCounter {
 public:
  SimpleFrameCounter(std::shared_ptr<Clock> clock,
                     int frame_min,
                     int frame_max,
                     int milliseconds);
  virtual ~SimpleFrameCounter();

  virtual int ReadFrame() override;

 private:
  float change_interval_;
  float time_at_last_check_;
};

// Loop frame counter that counts from frame_min to frame_max, starting over at
// frame_min.
class LoopFrameCounter : public FrameCounter {
 public:
  LoopFrameCounter(std::shared_ptr<Clock> clock,
                   int frame_min,
                   int frame_max,
                   int milliseconds);
  virtual ~LoopFrameCounter();

  virtual int ReadFrame() override;
  virtual void Finished() override;

 private:
  float change_interval_;
  float time_at_last_check_;
};

// Turn frame counter that counts from frame_min to frame_max and then counts
// back down to frame_min.
class TurnFrameCounter : public FrameCounter {
 public:
  TurnFrameCounter(std::shared_ptr<Clock> clock,
                   int frame_min,
                   int frame_max,
                   int milliseconds);
  virtual ~TurnFrameCounter();

  virtual int ReadFrame() override;

 private:
  bool going_forward_;
  unsigned int change_interval_;
  unsigned int time_at_last_check_;
};

// Frame counter that counts from frame_min to frame_max, speeding up as it
// goes.
class AcceleratingFrameCounter : public FrameCounter {
 public:
  AcceleratingFrameCounter(std::shared_ptr<Clock> clock,
                           int frame_min,
                           int frame_max,
                           int milliseconds);
  virtual ~AcceleratingFrameCounter();

  virtual int ReadFrame() override;

 private:
  unsigned int start_time_;
  float time_at_last_check_;
};

// Frame counter that counts from frame_min to frame_max, slowing down as it
// goes.
class DeceleratingFrameCounter : public FrameCounter {
 public:
  DeceleratingFrameCounter(std::shared_ptr<Clock> clock,
                           int frame_min,
                           int frame_max,
                           int milliseconds);
  virtual ~DeceleratingFrameCounter();

  virtual int ReadFrame() override;

 private:
  unsigned int start_time_;
  float time_at_last_check_;
};
