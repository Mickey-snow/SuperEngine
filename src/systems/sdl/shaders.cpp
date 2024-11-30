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

#include <GL/glew.h>

#include "systems/sdl/sdl_utils.hpp"
#include "systems/sdl/shaders.hpp"

static_assert(std::same_as<unsigned int, GLuint>);

static void checkCompileError(GLuint shader) {
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar msg[1024];
    glGetShaderInfoLog(shader, 1024, NULL, msg);
    throw std::runtime_error(std::string("Shader compile error: ") + msg);
  }
}

static void checkLinkError(GLuint program) {
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLchar msg[1024];
    glGetProgramInfoLog(program, 1024, NULL, msg);
    throw std::runtime_error(std::string("Shader linking error: ") + msg);
  }
}

ShaderProgram::ShaderProgram(std::string_view vertex_src,
                             std::string_view frag_src) {
  const char *vs = vertex_src.data(), *fs = frag_src.data();

  auto vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vs, NULL);
  glCompileShader(vertex);
  checkCompileError(vertex);

  auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fs, NULL);
  glCompileShader(fragment);
  checkCompileError(fragment);

  id_ = glCreateProgram();
  glAttachShader(id_, vertex);
  glAttachShader(id_, fragment);
  glLinkProgram(id_);
  checkLinkError(id_);

  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

ShaderProgram::~ShaderProgram() {
  if (id_ != 0) {
    glDeleteProgram(id_);
  }
}

unsigned int ShaderProgram::UniformLocation(std::string_view name) {
  auto location = glGetUniformLocation(id_, name.data());
  if (location == -1) {
    throw std::runtime_error("ShaderProgram: Uniform " + std::string(name) +
                             " not found.");
  }
  return location;
}

void ShaderProgram::SetUniform(std::string_view name, int value) {
  glUniform1i(UniformLocation(name), value);
}

void ShaderProgram::SetUniform(std::string_view name, float value) {
  glUniform1f(UniformLocation(name), value);
}

void ShaderProgram::SetUniform(std::string_view name,
                               float x,
                               float y,
                               float z,
                               float w) {
  glUniform4f(UniformLocation(name), x, y, z, w);
}

void ShaderProgram::SetUniform(std::string_view name,
                               float x,
                               float y,
                               float z) {
  glUniform3f(UniformLocation(name), x, y, z);
}

std::shared_ptr<ShaderProgram> GetOpShader() {
  static constexpr std::string_view vertex_src = R"glsl(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aOpacity;
layout (location = 2) in vec2 aTexCoord;

out float Opacity;
out vec2 TexCoord;

void main(){
  gl_Position = vec4(aPos, 0.0, 1.0);
  Opacity = aOpacity;
  TexCoord = aTexCoord;
}
)glsl";
  static constexpr std::string_view fragment_src = R"glsl(
#version 330 core
in float Opacity;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec4 mask_color;
out vec4 FragColor;

void main(){
  vec4 textureColor = texture(texture1, TexCoord);
  vec3 blend_color = clamp((textureColor.rgb + mask_color.rgb*mask_color.a), 0.0, 1.0);
  FragColor = vec4(blend_color, Opacity * textureColor.a);
}
)glsl";

  static auto shader =
      std::make_shared<ShaderProgram>(vertex_src, fragment_src);
  return shader;
}

std::shared_ptr<ShaderProgram> GetColorMaskShader() {
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
  vec4 bg_color = texture2D(texture0, TexCoord0);
  vec4 mask_sample = texture2D(texture1, TexCoord1);

  float mask_strength = clamp(mask_sample.a * color.a, 0.0, 1.0);
  vec4 blended_color = bg_color - mask_strength + color * mask_strength;
  FragColor = clamp(blended_color, 0.0, 1.0);
}
)glsl";

  static auto shader =
      std::make_shared<ShaderProgram>(vertex_src, fragment_src);
  return shader;
}

std::shared_ptr<ShaderProgram> GetObjectShader() {
  static constexpr std::string_view vertex_src = R"glsl(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main(){
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord = aTexCoord;
}
)glsl";
  static constexpr std::string_view fragment_src = R"glsl(
#version 330 core
in vec2 TexCoord;

uniform sampler2D image;
uniform vec4 colour;
uniform float mono;
uniform float invert;
uniform float light;
uniform vec3 tint;
uniform float alpha;

out vec4 FragColor;

void tinter(in float pixel_val, in float tint_val, out float mixed) {
  if (tint_val > 0.0) {
    mixed = pixel_val + tint_val - (pixel_val * tint_val);
  } else if (tint_val < 0.0) {
    mixed = pixel_val * abs(tint_val);
  } else {
    mixed = pixel_val;
  }
}

void main() {
  vec4 pixel = texture2D(image, TexCoord);

  // Blend with the input colour
  vec3 coloured = mix(pixel.rgb, colour.rgb, colour.a);
  pixel = vec4(coloured, pixel.a);

  // Apply grayscale effect
  if (mono > 0.0) {
    float gray = dot(pixel.rgb, vec3(0.299, 0.587, 0.114));
    vec3 mixed = mix(pixel.rgb, vec3(gray), mono);
    pixel.rgb = mixed;
  }

  // Apply inversion effect
  if (invert > 0.0) {
    vec3 inverted = vec3(1.0) - pixel.rgb;
    vec3 mixed = mix(pixel.rgb, inverted, invert);
    pixel.rgb = mixed;
  }

  // Apply lighting adjustment
  float out_r, out_g, out_b;
  tinter(pixel.r, light, out_r);
  tinter(pixel.g, light, out_g);
  tinter(pixel.b, light, out_b);
  pixel.rgb = vec3(out_r, out_g, out_b);

  // Apply tint
  tinter(pixel.r, tint.r, out_r);
  tinter(pixel.g, tint.g, out_g);
  tinter(pixel.b, tint.b, out_b);
  pixel.rgb = vec3(out_r, out_g, out_b);

  // Adjust alpha
  pixel.a *= alpha;
  FragColor = pixel;
}
)glsl";

  static auto shader =
      std::make_shared<ShaderProgram>(vertex_src, fragment_src);
  return shader;
}
