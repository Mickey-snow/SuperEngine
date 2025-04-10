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

#pragma once

#include <memory>
#include <string>

#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "object/animator.hpp"
#include "object/objdrawer.hpp"
#include "object/service_locator.hpp"

class System;
class SDLSurface;
class RLMachine;

// -----------------------------------------------------------------------

// GraphicsObjectData class that encapsulates a G00 or ANM file.
//
// GraphicsObjectOfFile is used for loading individual bitmaps into an
// object. It has support for normal display, and also
class GraphicsObjectOfFile : public GraphicsObjectData {
 public:
  GraphicsObjectOfFile(std::shared_ptr<SDLSurface> surface);
  virtual ~GraphicsObjectOfFile();

  virtual int PixelWidth(const GraphicsObject& rp) override;
  virtual int PixelHeight(const GraphicsObject& rp) override;

  virtual std::unique_ptr<GraphicsObjectData> Clone() const override;

  virtual void Execute(RLMachine& machine) override;
  void Execute();

  virtual void PlaySet(int set) override;

  virtual Animator const* GetAnimator() const override;
  virtual Animator* GetAnimator() override;

 protected:
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& go) override;
  virtual Rect SrcRect(const GraphicsObject& go) override;

 private:
  Animator animator_;

  // The encapsulated surface to render
  std::shared_ptr<Surface> surface_;

  // Number of milliseconds to spend on a single frame in the
  // animation
  unsigned int frame_time_;

  // Current frame displayed (when animating)
  int current_frame_;
};
