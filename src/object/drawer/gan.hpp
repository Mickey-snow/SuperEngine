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

#include "core/avdec/gan.hpp"
#include "object/animator.hpp"
#include "object/objdrawer.hpp"

#include <memory>
#include <string>

class SDLSurface;
using Surface = SDLSurface;
class System;
class RLMachine;
class GraphicsObject;

// -----------------------------------------------------------------------

// In-memory representation of a GAN file. Responsible for reading in,
// storing, and rendering GAN data as a GraphicsObjectData.
class GanGraphicsObjectData : public GraphicsObjectData {
 public:
  GanGraphicsObjectData(std::shared_ptr<Surface> image,
                        std::vector<std::vector<GanDecoder::Frame>> frames);
  virtual ~GanGraphicsObjectData();

  void LoadGANData();

  virtual int PixelWidth(const GraphicsObject& rendering_properties) override;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) override;

  virtual std::unique_ptr<GraphicsObjectData> Clone() const override;
  virtual void Execute(RLMachine& machine) override;

  virtual void PlaySet(int set) override;

  virtual Animator const* GetAnimator() const override { return &animator_; }
  virtual Animator* GetAnimator() override { return &animator_; }

 protected:
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& go) override;
  virtual Rect SrcRect(const GraphicsObject& go) override;
  virtual Point DstOrigin(const GraphicsObject& go) override;
  virtual int GetRenderingAlpha(const GraphicsObject& go,
                                const GraphicsObject* parent) override;

 private:
  Animator animator_;

  std::shared_ptr<Surface> image_;

  using Frame = GanDecoder::Frame;
  std::vector<std::vector<Frame>> animation_sets;

  int current_set_;
  int current_frame_;
  unsigned int delta_time_;
};
