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

#include "core/avdec/anm.hpp"
#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "object/animator.hpp"
#include "object/objdrawer.hpp"

#include <memory>
#include <string>
#include <vector>

class SDLSurface;
using Surface = SDLSurface;
class System;

// Executable, in-memory representation of an ANM file. This internal structure
// is heavily based off of xkanon's ANM file implementation, but has been
// changed to be all C++ like.
class AnmGraphicsObjectData : public GraphicsObjectData {
 public:
  AnmGraphicsObjectData(std::shared_ptr<Surface> surface, AnmDecoder anm_data);
  virtual ~AnmGraphicsObjectData();

  virtual int PixelWidth(const GraphicsObject& rendering_properties) override;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) override;

  virtual std::unique_ptr<GraphicsObjectData> Clone() const override;
  virtual void Execute(RLMachine& machine) override;

  virtual void PlaySet(int set) override;

 protected:
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& go) override;
  virtual Rect SrcRect(const GraphicsObject& go) override;
  virtual Rect DstRect(const GraphicsObject& go,
                       const GraphicsObject* parent) override;

  virtual Animator const* GetAnimator() const override { return &animator_; }
  virtual Animator* GetAnimator() override { return &animator_; }

 private:
  // Advance the position in the animation.
  void AdvanceFrame();
  using Frame = AnmDecoder::Frame;

  Animator animator_;

  // Animation Data (This structure was stolen from xkanon.)
  std::vector<Frame> frames;
  std::vector<std::vector<int>> framelist_;
  std::vector<std::vector<int>> animation_set_;

  // The image the above coordinates map into.
  std::shared_ptr<Surface> image_;

  bool currently_playing_;
  int current_set_;
  unsigned int delta_time_;

  // iterators of animation_set_
  std::vector<int>::const_iterator cur_frame_set_;
  std::vector<int>::const_iterator cur_frame_set_end_;

  // iterators of framelist_
  std::vector<int>::const_iterator cur_frame_;
  std::vector<int>::const_iterator cur_frame_end_;

  int current_frame_;
};
