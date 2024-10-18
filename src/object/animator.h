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
  // virtual void EndAnimation() = 0;
  // virtual void LoopAnimation() = 0;
};

class GraphicsObjectData;

class Animator : public IAnimator {
 public:
  // TODO: just a reminder to decouple them later
  friend class GraphicsObjectData;

 public:
  Animator()
      : after_animation_(AFTER_NONE),
        currently_playing_(false),
        animation_finished_(false) {}

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

 private:
  // Policy of what to do after an animation is finished.
  AfterAnimation after_animation_;

  bool currently_playing_;

  // Whether we're on the final frame.
  bool animation_finished_;

  // boost::serialization support
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & after_animation_ & currently_playing_;
  }
};

#endif
