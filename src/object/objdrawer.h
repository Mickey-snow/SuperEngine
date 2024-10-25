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
class Animator;

class GraphicsObjectData {
 public:
  GraphicsObjectData();
  virtual ~GraphicsObjectData();

  virtual void PlaySet(int set);

  virtual void Render(const GraphicsObject& go, const GraphicsObject* parent);

  virtual int PixelWidth(const GraphicsObject& rendering_properties) = 0;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) = 0;
  // Returns the destination rectangle on the screen to draw srcRect()
  // to. Override to return custom rectangles in the case of a custom animation
  // format.
  virtual Rect DstRect(const GraphicsObject& go, const GraphicsObject* parent);

  virtual std::unique_ptr<GraphicsObjectData> Clone() const = 0;

  virtual void Execute(RLMachine& machine) = 0;

  // Whether this object data owns another layer of objects.
  virtual bool IsParentLayer() const;

  bool IsAnimation() const { return GetAnimator() != nullptr; }
  virtual Animator const* GetAnimator() const { return nullptr; }
  virtual Animator* GetAnimator() { return nullptr; }

 protected:
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

 protected:
  // boost::serialization support
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {}
};

#endif
