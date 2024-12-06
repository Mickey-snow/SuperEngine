// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "systems/glshaders.hpp"

#include "systems/sdl/sdl_utils.hpp"

#include <GL/glew.h>

std::shared_ptr<glslProgram> _GetColorMaskShader() {
  static constexpr std::string_view vertex_src = R"glsl(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord0;
layout (location = 2) in vec2 aTexCoord1;

out vec2 TexCoord0;
out vec2 TexCoord1;

void main(){
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord0 = aTexCoord0;
  TexCoord1 = aTexCoord1;
}
)glsl";
  static constexpr std::string_view fragment_src = R"glsl(
#version 330 core

in vec2 TexCoord0;
in vec2 TexCoord1;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec4 color;
out vec4 FragColor;

void main(){
  vec4 col = color / 255.0;

  vec4 bg_color = texture(texture0, TexCoord0);
  vec4 mask_sample = texture(texture1, TexCoord1);

  float mask_strength = clamp(mask_sample.a * col.a, 0.0, 1.0);
  vec4 blended_color = bg_color - mask_strength + col * mask_strength;
  FragColor = clamp(blended_color, 0.0, 1.0);
}
)glsl";

  static auto shader = std::make_shared<glslProgram>(vertex_src, fragment_src);
  return shader;
}
