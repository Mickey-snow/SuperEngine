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

enum AfterAnimation { AFTER_NONE, AFTER_CLEAR, AFTER_LOOP };

class GraphicsObjectData;

class Animator {
 public:
  // TODO: just a reminder to decouple them later
  friend class GraphicsObjectData;

 public:
  Animator()
      : after_animation_(AFTER_NONE),
        currently_playing_(false),
        animation_finished_(false) {}

  void SetAfterAction(AfterAnimation after) { after_animation_ = after; }
  AfterAnimation GetAfterAction() const { return after_animation_; }
  void SetIsPlaying(bool in) { currently_playing_ = in; }
  bool IsPlaying() const { return currently_playing_; }
  bool IsFinished() const { return animation_finished_; }
  void SetIsFinished(bool in) { animation_finished_ = in; }

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
