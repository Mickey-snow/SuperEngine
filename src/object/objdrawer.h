// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#ifndef SRC_OBJECT_DRAWER_H_
#define SRC_OBJECT_DRAWER_H_

#include <boost/serialization/access.hpp>

#include <memory>
#include <string>
#include <vector>

class GraphicsObject;
class Point;
class RLMachine;
class Rect;
class Surface;

class GraphicsObjectData {
 public:
  enum AfterAnimation { AFTER_NONE, AFTER_CLEAR, AFTER_LOOP };

 public:
  GraphicsObjectData();
  explicit GraphicsObjectData(const GraphicsObjectData& obj);
  virtual ~GraphicsObjectData();

  void set_after_action(AfterAnimation after) {
    animator_.after_animation_ = after;
  }
  AfterAnimation get_after_action() const { return animator_.after_animation_; }
  void set_is_currently_playing(bool in) { animator_.currently_playing_ = in; }
  bool is_currently_playing() const { return animator_.currently_playing_; }
  bool is_animation_finished() const { return animator_.animation_finished_; }
  void set_animation_finished(bool in) { animator_.animation_finished_ = in; }

  virtual bool IsAnimation() const;
  virtual void PlaySet(int set);

  virtual void Render(const GraphicsObject& go, const GraphicsObject* parent);

  virtual int PixelWidth(const GraphicsObject& rendering_properties) = 0;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) = 0;
  // Returns the destination rectangle on the screen to draw srcRect()
  // to. Override to return custom rectangles in the case of a custom animation
  // format.
  virtual Rect DstRect(const GraphicsObject& go, const GraphicsObject* parent);

  virtual GraphicsObjectData* Clone() const = 0;

  virtual void Execute(RLMachine& machine) = 0;

  // Whether this object data owns another layer of objects.
  virtual bool IsParentLayer() const;

 protected:
  // Function called after animation ends when this object has been
  // set up to loop. Default implementation does nothing.
  virtual void LoopAnimation();

  // Takes the specified action when we've reached the last frame of
  // animation.
  void EndAnimation();

  // Template method used during rendering to get the surface to render.
  // Return a null shared_ptr to disable rendering.
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& rp) = 0;

  // Returns the rectangle in currentSurface() to draw to the screen. Override
  // to return custom rectangles in the case of a custom animation format.
  virtual Rect SrcRect(const GraphicsObject& go);

  // Returns the offset to the destination, which is set on a per surface
  // basis. This template method can be ignored if you override dstRect().
  virtual Point DstOrigin(const GraphicsObject& go);

  // Controls the alpha during rendering. Default implementation just consults
  // the GraphicsObject.
  virtual int GetRenderingAlpha(const GraphicsObject& go,
                                const GraphicsObject* parent);

  class Animator {
   public:
    Animator()
        : after_animation_(AFTER_NONE),
          currently_playing_(false),
          animation_finished_(false) {}

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
  Animator const& GetAnimator() const { return animator_; }

 private:
  Animator animator_;

  friend class boost::serialization::access;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {}
};

#endif
