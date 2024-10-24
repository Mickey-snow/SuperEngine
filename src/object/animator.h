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

#ifndef SRC_OBJECT_ANIMATOR_H_
#define SRC_OBJECT_ANIMATOR_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>

#include <chrono>
#include <functional>

enum AfterAnimation { AFTER_NONE, AFTER_CLEAR, AFTER_LOOP };

class IAnimator {
 public:
  virtual ~IAnimator() = default;

  virtual void SetAfterAction(AfterAnimation after) = 0;
  virtual AfterAnimation GetAfterAction() const = 0;
  virtual void SetIsPlaying(bool in) = 0;
  virtual bool IsPlaying() const = 0;
  virtual bool IsFinished() const = 0;
  virtual void SetIsFinished(bool in) = 0;
};

class GraphicsObjectData;

class Animator : public IAnimator {
 public:
  // TODO: just a reminder to decouple them later
  friend class GraphicsObjectData;

  using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;
  using duration_t = std::chrono::milliseconds;

 public:
  Animator()
      : after_animation_(AFTER_NONE),
        state_(State::Paused),
        currently_playing_(false),
        animation_finished_(false),
        animation_time_(duration_t::zero()) {}

  virtual void SetAfterAction(AfterAnimation after) override {
    after_animation_ = after;
  }
  virtual AfterAnimation GetAfterAction() const override {
    return after_animation_;
  }
  virtual void SetIsPlaying(bool in) override { currently_playing_ = in; }
  virtual bool IsPlaying() const override { return currently_playing_; }
  virtual bool IsFinished() const override { return animation_finished_; }
  virtual void SetIsFinished(bool in) override { animation_finished_ = in; }

  enum class Action { Pause, Play, Stop };
  virtual void Apply(Action action, timepoint_t now) {
    Notify(now);

    switch (action) {
      case Action::Pause: {
        if (state_ != State::Finished)
          state_ = State::Paused;
      } break;

      case Action::Play:
        state_ = State::Playing;
        break;

      case Action::Stop: {
        animation_time_ = decltype(animation_time_)::zero();
        state_ = State::Finished;
      } break;

      default:
        throw std::invalid_argument("Animator: invalid action " +
                                    std::to_string(static_cast<int>(action)));
    }
  }

  enum class State { Paused, Playing, Finished };
  State GetState(void) const { return state_; }

  duration_t GetAnmTime() const { return animation_time_; }

  void Notify(timepoint_t tp) {
    if (state_ == State::Playing)
      animation_time_ +=
          std::chrono::duration_cast<duration_t>(tp - last_tick_);
    last_tick_ = tp;
  }

 private:
  // Policy of what to do after an animation is finished.
  AfterAnimation after_animation_;  // dummy

  State state_;

  bool currently_playing_;   // dummy
  bool animation_finished_;  // dummy

  timepoint_t last_tick_;
  duration_t animation_time_;

  // boost::serialization support
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & after_animation_ & currently_playing_;
  }
};

#endif
