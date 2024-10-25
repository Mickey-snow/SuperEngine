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

#include "utilities/stopwatch.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include <chrono>
#include <functional>

enum AfterAnimation { AFTER_NONE, AFTER_CLEAR, AFTER_LOOP };

class Animator {
 public:
  explicit Animator(std::shared_ptr<Clock> clock)
      : after_(AFTER_NONE), timer_(clock) {}
  virtual ~Animator() = default;

  virtual void SetAfterAction(AfterAnimation after) { after_ = after; }
  virtual AfterAnimation GetAfterAction() const { return after_; }
  virtual void SetIsPlaying(bool in) {
    if (in) {
      timer_.Apply(Stopwatch::Action::Run);
    } else
      timer_.Apply(Stopwatch::Action::Pause);
  }
  virtual bool IsPlaying() const {
    return timer_.GetState() == Stopwatch::State::Running;
  }
  virtual bool IsFinished() const {
    return timer_.GetState() == Stopwatch::State::Stopped;
  }
  virtual void SetIsFinished(bool in) {
    if (in)
      timer_.Apply(Stopwatch::Action::Reset);
  }

  virtual Stopwatch::duration_t GetAnimationTime() const {
    return timer_.GetReading();
  }

 private:
  AfterAnimation after_;
  mutable Stopwatch
      timer_;  // mutable because internal state is coupled with time

  // boost::serialization support
  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar & after_& IsPlaying();
  }
  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar & after_;
    bool is_playing;
    ar & is_playing;
    if (is_playing)
      timer_.Apply(Stopwatch::Action::Run);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER();
};

#endif
