// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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
// -----------------------------------------------------------------------

#pragma once

#include "core/object_internal/objdrawer.hpp"
#include "systems/sdl/sdl_surface.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

class MockGraphicsObjectData : public GraphicsObjectData {
 public:
  using RenderCallback =
      std::function<void(const GraphicsObject&, std::optional<ParentObjState>)>;

  MockGraphicsObjectData() : src_(Rect::REC(0, 0, 1, 1)) {}

  explicit MockGraphicsObjectData(std::shared_ptr<const SDLSurface> surface)
      : src_(Rect::REC(0,
                       0,
                       surface->GetSize().width(),
                       surface->GetSize().height())),
        surface_(std::move(surface)) {}

  MockGraphicsObjectData(Rect src,
                         Point texture_origin,
                         std::shared_ptr<const SDLSurface> surface)
      : src_(src),
        texture_origin_(texture_origin),
        surface_(std::move(surface)) {}

  void SetRenderCallback(RenderCallback callback) {
    render_callback_ = std::move(callback);
  }

  void Render(const GraphicsObject& object,
              std::optional<ParentObjState> parent) override {
    if (render_callback_)
      render_callback_(object, std::move(parent));
    else
      GraphicsObjectData::Render(object, std::move(parent));
  }

  int PixelWidth(const GraphicsObject&) override { return src_.width(); }
  int PixelHeight(const GraphicsObject&) override { return src_.height(); }

  std::unique_ptr<GraphicsObjectData> Clone() const override {
    return std::make_unique<MockGraphicsObjectData>(*this);
  }

 protected:
  std::shared_ptr<const SDLSurface> CurrentSurface(
      const GraphicsObject&) const override {
    return surface_;
  }

  Rect SrcRect(const GraphicsObject&) const override { return src_; }

  Point DstOrigin(const GraphicsObject&) const override {
    return texture_origin_;
  }

 private:
  Rect src_;
  Point texture_origin_;
  std::shared_ptr<const SDLSurface> surface_;
  RenderCallback render_callback_;
};
