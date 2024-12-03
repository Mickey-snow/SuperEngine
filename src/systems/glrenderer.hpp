// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "base/colour.hpp"
#include "base/rect.hpp"
#include "glm/mat4x4.hpp"

#include <array>
#include <memory>
#include <optional>

class glTexture;
class ShaderProgram;
class glFrameBuffer;

struct glRenderable {
  std::shared_ptr<glTexture> texture_;
  Rect region;
};

struct RenderingConfig {
  Rect dst;
  std::optional<glm::mat4> model;
  std::optional<RGBAColour> colour;
  std::optional<float> mono;
  std::optional<float> invert;
  std::optional<float> light;
  std::optional<RGBColour> tint;
  std::optional<float> alpha;
  std::optional<std::array<float, 4>> vertex_alpha;
};

class glRenderer {
 public:
  glRenderer() = default;

  void SetUp();

  void SetFrameBuffer(std::shared_ptr<glFrameBuffer>);
  void ClearBuffer(RGBAColour color);

  // void SetView(glm::mat4 transformation);
  // void SetProjection(glm::mat4 transformation);

  // void RenderColormask(glRenderable, Rect dst, const RGBAColour mask);
  // void Render(glRenderable, RenderingConfig config);

 private:
  std::shared_ptr<glFrameBuffer> canvas_;
  glm::mat4 view_, projection_;
  std::shared_ptr<ShaderProgram> shader_;

  // struct glCtx;
  // std::unique_ptr<glCtx> ctx_;
};
