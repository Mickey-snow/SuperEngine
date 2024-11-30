// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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

#include "GL/glew.h"

#ifndef NDEBUG
#include <iostream>
#endif

#include "systems/base/graphics_object.hpp"
#include "systems/base/system_error.hpp"
#include "systems/sdl/sdl_utils.hpp"
#include "systems/sdl/shaders.hpp"

namespace {

const char kColorMaskShader[] =
    "uniform sampler2D current_values, mask;"
    ""
    "void main()"
    "{"
    "vec4 bg_colour = texture2D(current_values, gl_TexCoord[0].st);"
    "vec4 mask_vector = texture2D(mask, gl_TexCoord[1].st);"
    "float mask_colour = clamp(mask_vector.a * gl_Color.a, 0.0, 1.0);"
    "gl_FragColor = clamp(bg_colour - mask_colour + gl_Color * mask_colour, "
    "                     0.0, 1.0);"
    "}";

const char kObjectShader[] =
    "uniform sampler2D image;\n"
    "uniform vec4 colour;\n"
    "uniform float mono;\n"
    "uniform float invert;\n"
    "uniform float light;\n"
    "uniform vec3 tint;\n"
    "uniform float alpha;\n"
    "\n"
    "void tinter(in float pixel_val, in float tint_val, out float mixed) {\n"
    "  if (tint_val > 0.0) {\n"
    "    mixed = pixel_val + tint_val - (pixel_val * tint_val);\n"
    "  } else if (tint_val < 0.0) {\n"
    "    mixed = pixel_val * abs(tint_val);\n"
    "  } else {\n"
    "    mixed = pixel_val;\n"
    "  }\n"
    "}\n"
    "\n"
    "void main() {\n"
    "  vec4 pixel = texture2D(image, gl_TexCoord[0].st);\n"
    "\n"
    "  // The colour is blended directly with the incoming pixel value.\n"
    "  vec3 coloured = mix(pixel.rgb, colour.rgb, colour.a);\n"
    "  pixel = vec4(coloured.r, coloured.g, coloured.b, pixel.a);\n"
    "\n"
    "  if (mono > 0.0) {\n"
    "    // NTSC grayscale\n"
    "    float gray = dot(pixel.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    vec3 mixed = mix(pixel.rgb, vec3(gray, gray, gray), mono);\n"
    "    pixel = vec4(mixed.r, mixed.g, mixed.b, pixel.a);\n"
    "  }\n"
    "\n"
    "  if (invert > 0.0) {\n"
    "    vec3 inverted = vec3(1.0, 1.0, 1.0) - pixel.rgb;\n"
    "    vec3 mixed = mix(pixel.rgb, inverted, invert);\n"
    "    pixel = vec4(mixed.r, mixed.g, mixed.b, pixel.a);\n"
    "  }\n"
    "\n"
    "  float out_r, out_g, out_b;\n"
    "  tinter(pixel.r, light, out_r);\n"
    "  tinter(pixel.g, light, out_g);\n"
    "  tinter(pixel.b, light, out_b);\n"
    "  pixel = vec4(out_r, out_g, out_b, pixel.a);\n"
    "\n"
    "  tinter(pixel.r, tint.r, out_r);\n"
    "  tinter(pixel.g, tint.g, out_g);\n"
    "  tinter(pixel.b, tint.b, out_b);\n"
    "  pixel = vec4(out_r, out_g, out_b, pixel.a);\n"
    "\n"
    "  // We're responsible for doing the main alpha blending, too.\n"
    "  pixel.a = pixel.a * alpha;\n"
    "  gl_FragColor = pixel;\n"
    "}\n";

}  // namespace

GLuint Shaders::color_mask_program_object_id_ = 0;
GLint Shaders::color_mask_current_values_ = 0;
GLint Shaders::color_mask_mask_ = 0;

GLuint Shaders::object_program_object_id_ = 0;
GLint Shaders::object_image_ = 0;
GLint Shaders::object_colour_ = 0;
GLint Shaders::object_tint_ = 0;
GLint Shaders::object_light_ = 0;
GLint Shaders::object_alpha_ = 0;
GLint Shaders::object_mono_ = 0;
GLint Shaders::object_invert_ = 0;

// static
void Shaders::Reset() {
  if (color_mask_program_object_id_) {
    glDeleteObjectARB(color_mask_program_object_id_);
    DebugShowGLErrors();

    color_mask_program_object_id_ = 0;
    color_mask_current_values_ = 0;
    color_mask_mask_ = 0;
  }

  if (object_program_object_id_) {
    glDeleteObjectARB(object_program_object_id_);
    DebugShowGLErrors();

    object_program_object_id_ = 0;
    object_image_ = 0;
    object_colour_ = 0;
    object_tint_ = 0;
    object_light_ = 0;
    object_alpha_ = 0;
    object_mono_ = 0;
    object_invert_ = 0;
  }
}

// static
GLuint Shaders::getColorMaskProgram() {
  if (color_mask_program_object_id_ == 0) {
    buildShader(kColorMaskShader, &color_mask_program_object_id_);
  }

  return color_mask_program_object_id_;
}

GLint Shaders::getColorMaskUniformCurrentValues() {
  if (color_mask_current_values_ == 0) {
    color_mask_current_values_ =
        glGetUniformLocationARB(getColorMaskProgram(), "current_values");
    if (color_mask_current_values_ == -1)
      throw SystemError("Bad uniform value: current_values");
  }

  return color_mask_current_values_;
}

GLint Shaders::getColorMaskUniformMask() {
  if (color_mask_mask_ == 0) {
    color_mask_mask_ = glGetUniformLocationARB(getColorMaskProgram(), "mask");
    if (color_mask_mask_ == -1)
      throw SystemError("Bad uniform value: mask");
  }

  return color_mask_mask_;
}

GLuint Shaders::GetObjectProgram() {
  if (object_program_object_id_ == 0) {
    buildShader(kObjectShader, &object_program_object_id_);
  }

  return object_program_object_id_;
}

GLint Shaders::GetObjectUniformImage() {
  if (object_image_ == 0) {
    object_image_ = glGetUniformLocationARB(GetObjectProgram(), "image");
    if (object_image_ == -1)
      throw SystemError("Bad uniform value: image");
  }

  return object_image_;
}

void Shaders::loadObjectUniformFromGraphicsObject(const GraphicsObject& go) {
  auto& param = go.Param();

  RGBAColour colour = param.colour();
  glUniform4fARB(Shaders::GetObjectUniformColour(), colour.r_float(),
                 colour.g_float(), colour.b_float(), colour.a_float());

  RGBColour tint = param.tint();
  glUniform3fARB(Shaders::GetObjectUniformTint(), tint.r_float(),
                 tint.g_float(), tint.b_float());

  glUniform1fARB(Shaders::GetObjectUniformLight(), param.light() / 255.0f);

  glUniform1fARB(Shaders::GetObjectUniformMono(), param.mono() / 255.0f);

  glUniform1fARB(Shaders::GetObjectUniformInvert(), param.invert() / 255.0f);
}

GLint Shaders::GetObjectUniformColour() {
  if (object_colour_ == 0) {
    object_colour_ = glGetUniformLocationARB(GetObjectProgram(), "colour");
    if (object_colour_ == -1)
      throw SystemError("Bad uniform value: colour");
  }

  return object_colour_;
}

GLint Shaders::GetObjectUniformTint() {
  if (object_tint_ == 0) {
    object_tint_ = glGetUniformLocationARB(GetObjectProgram(), "tint");
    if (object_tint_ == -1)
      throw SystemError("Bad uniform value: tint");
  }

  return object_tint_;
}

GLint Shaders::GetObjectUniformLight() {
  if (object_light_ == 0) {
    object_light_ = glGetUniformLocationARB(GetObjectProgram(), "light");
    if (object_light_ == -1)
      throw SystemError("Bad uniform value: light");
  }

  return object_light_;
}

GLint Shaders::GetObjectUniformAlpha() {
  if (object_alpha_ == 0) {
    object_alpha_ = glGetUniformLocationARB(GetObjectProgram(), "alpha");
    if (object_alpha_ == -1)
      throw SystemError("Bad uniform value: alpha");
  }

  return object_alpha_;
}

GLint Shaders::GetObjectUniformMono() {
  if (object_mono_ == 0) {
    object_mono_ = glGetUniformLocationARB(GetObjectProgram(), "mono");
    if (object_mono_ == -1)
      throw SystemError("Bad uniform value: mono");
  }

  return object_mono_;
}

GLint Shaders::GetObjectUniformInvert() {
  if (object_invert_ == 0) {
    object_invert_ = glGetUniformLocationARB(GetObjectProgram(), "invert");
    if (object_invert_ == -1)
      throw SystemError("Bad uniform value: invert");
  }

  return object_invert_;
}

// static
void Shaders::buildShader(const char* shader, GLuint* program_object) {
  GLuint shader_object = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
  DebugShowGLErrors();

  glShaderSourceARB(shader_object, 1, &shader, NULL);
  DebugShowGLErrors();

  glCompileShaderARB(shader_object);
  DebugShowGLErrors();

#ifndef NDEBUG
  GLint blen = 0;
  GLsizei slen = 0;
  glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &blen);
  if (blen > 1) {
    GLchar* compiler_log = new GLchar[blen];
    glGetInfoLogARB(shader_object, blen, &slen, compiler_log);
    std::cout << "compiler_log: " << std::endl << compiler_log << std::endl;
    delete[] compiler_log;
  }
#endif

  *program_object = glCreateProgramObjectARB();
  glAttachObjectARB(*program_object, shader_object);
  DebugShowGLErrors();

  glLinkProgramARB(*program_object);
  glDeleteObjectARB(shader_object);
  ShowGLErrors();
}

// -----------------------------------------------------------------------
static_assert(std::same_as<unsigned int, GLuint>);

void checkCompileError(GLuint shader) {
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar msg[1024];
    glGetShaderInfoLog(shader, 1024, NULL, msg);
    throw std::runtime_error(std::string("Shader compile error: ") + msg);
  }
}

void checkLinkError(GLuint program) {
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

void ShaderProgram::SetUniform(std::string_view name, float value) {
  auto location = glGetUniformLocation(id_, name.data());
  if (location == -1) {
    throw std::runtime_error("ShaderProgram: Uniform " + std::string(name) +
                             " not found.");
  }
  glUniform1i(location, value);
}

void ShaderProgram::SetUniform(std::string_view name,
                               float x,
                               float y,
                               float z,
                               float w) {
  auto location = glGetUniformLocation(id_, name.data());
  if (location == -1) {
    throw std::runtime_error("ShaderProgram: Uniform " + std::string(name) +
                             " not found.");
  }

  glUniform4f(location, x, y, z, w);
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
