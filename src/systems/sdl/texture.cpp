// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include <SDL/SDL.h>
#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "object/objdrawer.hpp"
#include "pygame/alphablit.h"
#include "systems/base/colour.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/system_error.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_surface.hpp"
#include "systems/sdl/sdl_utils.hpp"
#include "systems/sdl/shaders.hpp"
#include "systems/sdl/texture.hpp"

unsigned int Texture::s_screen_width = 0;
unsigned int Texture::s_screen_height = 0;

unsigned int Texture::s_upload_buffer_size = 0;
std::unique_ptr<char[]> Texture::s_upload_buffer;

inline static auto Normalize(auto value, auto max) {
  return static_cast<float>(value) / static_cast<float>(max);
}

// -----------------------------------------------------------------------

void Texture::SetScreenSize(const Size& s) {
  s_screen_width = s.width();
  s_screen_height = s.height();
}

int Texture::ScreenHeight() { return s_screen_height; }

// -----------------------------------------------------------------------
// Texture
// -----------------------------------------------------------------------
Texture::Texture(SDL_Surface* surface,
                 int x,
                 int y,
                 int w,
                 int h,
                 unsigned int bytes_per_pixel,
                 int byte_order,
                 int byte_type)
    : x_offset_(x),
      y_offset_(y),
      logical_width_(w),
      logical_height_(h),
      total_width_(surface->w),
      total_height_(surface->h),
      texture_width_(SafeSize(logical_width_)),
      texture_height_(SafeSize(logical_height_)),
      back_texture_id_(0),
      is_upside_down_(false) {
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  DebugShowGLErrors();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (w == total_width_ && h == total_height_) {
    SDL_LockSurface(surface);
    glTexImage2D(GL_TEXTURE_2D, 0, bytes_per_pixel, texture_width_,
                 texture_height_, 0, byte_order, byte_type, NULL);
    DebugShowGLErrors();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h, byte_order,
                    byte_type, surface->pixels);
    DebugShowGLErrors();

    SDL_UnlockSurface(surface);
  } else {
    // Cut out the current piece
    char* pixel_data = uploadBuffer(surface->format->BytesPerPixel * w * h);
    char* cur_dst_ptr = pixel_data;

    SDL_LockSurface(surface);
    {
      char* cur_src_ptr = (char*)surface->pixels;
      cur_src_ptr += surface->pitch * y;

      int row_start = surface->format->BytesPerPixel * x;
      int subrow_size = surface->format->BytesPerPixel * w;
      for (int current_row = 0; current_row < h; ++current_row) {
        memcpy(cur_dst_ptr, cur_src_ptr + row_start, subrow_size);
        cur_dst_ptr += subrow_size;
        cur_src_ptr += surface->pitch;
      }
    }
    SDL_UnlockSurface(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, bytes_per_pixel, texture_width_,
                 texture_height_, 0, byte_order, byte_type, NULL);
    DebugShowGLErrors();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, byte_order, byte_type,
                    pixel_data);
    DebugShowGLErrors();
  }
}

// -----------------------------------------------------------------------

Texture::Texture(render_to_texture, int width, int height)
    : x_offset_(0),
      y_offset_(0),
      logical_width_(width),
      logical_height_(height),
      total_width_(width),
      total_height_(height),
      texture_width_(0),
      texture_height_(0),
      texture_id_(0),
      back_texture_id_(0),
      is_upside_down_(true) {
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  DebugShowGLErrors();
  //  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  texture_width_ = SafeSize(logical_width_);
  texture_height_ = SafeSize(logical_height_);

  // This may fail.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width_, texture_height_, 0,
               GL_RGB, GL_UNSIGNED_BYTE, NULL);
  DebugShowGLErrors();

  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, logical_width_,
                      logical_height_);
  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

Texture::~Texture() {
  glDeleteTextures(1, &texture_id_);

  if (back_texture_id_)
    glDeleteTextures(1, &back_texture_id_);

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

char* Texture::uploadBuffer(unsigned int size) {
  if (!s_upload_buffer || size > s_upload_buffer_size) {
    s_upload_buffer.reset(new char[size]);
    s_upload_buffer_size = size;
  }

  return s_upload_buffer.get();
}

// -----------------------------------------------------------------------

void Texture::reupload(SDL_Surface* surface,
                       int offset_x,
                       int offset_y,
                       int x,
                       int y,
                       int w,
                       int h,
                       unsigned int bytes_per_pixel,
                       int byte_order,
                       int byte_type) {
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  if (w == total_width_ && h == total_height_) {
    SDL_LockSurface(surface);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h, byte_order,
                    byte_type, surface->pixels);
    DebugShowGLErrors();

    SDL_UnlockSurface(surface);
  } else {
    // Cut out the current piece
    char* pixel_data = uploadBuffer(surface->format->BytesPerPixel * w * h);
    char* cur_dst_ptr = pixel_data;

    SDL_LockSurface(surface);
    {
      char* cur_src_ptr = (char*)surface->pixels;
      cur_src_ptr += surface->pitch * y;

      int row_start = surface->format->BytesPerPixel * x;
      int subrow_size = surface->format->BytesPerPixel * w;
      for (int current_row = 0; current_row < h; ++current_row) {
        memcpy(cur_dst_ptr, cur_src_ptr + row_start, subrow_size);
        cur_dst_ptr += subrow_size;
        cur_src_ptr += surface->pitch;
      }
    }
    SDL_UnlockSurface(surface);

    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, w, h, byte_order,
                    byte_type, pixel_data);
    DebugShowGLErrors();
  }
}

// -----------------------------------------------------------------------

void Texture::RenderToScreen(const Rect& src,
                             const Rect& dst,
                             const int opacity) {
  const float op = Normalize(opacity, 255);
  RenderToScreen(src, dst, {op, op, op, op});
}

// -----------------------------------------------------------------------

void Texture::RenderToScreen(const Rect& src,
                             const Rect& dst,
                             const int opacity[4]) {
  RenderToScreen(src, dst,
                 {Normalize(opacity[0], 255.0), Normalize(opacity[1], 255.0),
                  Normalize(opacity[2], 255.0), Normalize(opacity[3], 255.0)});
}

// -----------------------------------------------------------------------

void Texture::RenderToScreen(const Rect& src,
                             const Rect& dst,
                             std::array<float, 4> opacity,
                             RGBAColour color) {
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  auto toNDC = [w = s_screen_width, h = s_screen_height](int x, int y) {
    return std::make_pair(2.0f * x / w - 1.0f, 1.0f - (2.0f * y / h));
  };
  auto [dx1, dy1] = toNDC(fdx1, fdy1);
  auto [dx2, dy2] = toNDC(fdx2, fdy2);

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  float vertices[] = {
      dx1, dy1, opacity[0], thisx1, thisy1,  // NOLINT
      dx2, dy1, opacity[1], thisx2, thisy1,  // NOLINT
      dx2, dy2, opacity[2], thisx2, thisy2,  // NOLINT
      dx1, dy2, opacity[3], thisx1, thisy2   // NOLINT
  };
  unsigned int indices[] = {0, 1, 2, 0, 2, 3};

  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STREAM_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);

  auto shader = GetOpShader();
  glUseProgram(shader->GetID());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  shader->SetUniform("texture1", 0);
  shader->SetUniform("mask_color", Normalize(color.r(), 255),
                     Normalize(color.g(), 255), Normalize(color.b(), 255),
                     Normalize(color.a(), 255));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glUseProgram(0);
  glBindVertexArray(0);
  glBlendFunc(GL_ONE, GL_ZERO);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

// For rendering text waku and select buttons
// TODO(erg): A function of this hairiness needs super more amounts of
// documentation.
void Texture::RenderToScreenAsColorMask(const Rect& src,
                                        const Rect& dst,
                                        const RGBAColour& rgba,
                                        int is_filter) {
  if (is_filter == 0) {
    RenderSubtractiveColorMask(src, dst, rgba);
  } else {
    RenderToScreen(src, dst, {1.0, 1.0, 1.0, 1.0}, rgba);
  }
}

// -----------------------------------------------------------------------

void Texture::RenderSubtractiveColorMask(const Rect& src,
                                         const Rect& dst,
                                         const RGBAColour& color) {
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  auto toNDC = [w = s_screen_width, h = s_screen_height](int x, int y) {
    return std::make_pair(2.0f * x / w - 1.0f, 1.0f - (2.0f * y / h));
  };
  auto [dx1, dy1] = toNDC(fdx1, fdy1);
  auto [dx2, dy2] = toNDC(fdx2, fdy2);

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  float vertices[] = {
      dx1, dy1, thisx1, thisy2, thisx1, thisy1,  // NOLINT
      dx2, dy1, thisx2, thisy2, thisx2, thisy1,  // NOLINT
      dx2, dy2, thisx2, thisy1, thisx2, thisy2,  // NOLINT
      dx1, dy2, thisx1, thisy1, thisx1, thisy2   // NOLINT
  };
  unsigned int indices[] = {0, 1, 2, 0, 2, 3};
  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STREAM_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)(4 * sizeof(float)));
  glEnableVertexAttribArray(2);

  if (back_texture_id_ == 0) {
    glGenTextures(1, &back_texture_id_);
    glBindTexture(GL_TEXTURE_2D, back_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Generate this texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width_, texture_height_, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, NULL);
    DebugShowGLErrors();
  }
  // Copy the current value of the region where we're going to render
  // to a texture for input to the shader
  glBindTexture(GL_TEXTURE_2D, back_texture_id_);
  int ystart = int(s_screen_height - fdy1 - (fdy2 - fdy1));
  int idx1 = int(fdx1);
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, idx1, ystart, texture_width_,
                      texture_height_);
  DebugShowGLErrors();

  auto shader = GetColorMaskShader();
  glUseProgram(shader->GetID());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, back_texture_id_);
  shader->SetUniform("texture0", 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  shader->SetUniform("texture1", 1);
  shader->SetUniform("color", Normalize(color.r(), 255),
                     Normalize(color.g(), 255), Normalize(color.b(), 255),
                     Normalize(color.a(), 255));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glUseProgram(0);
  glBindVertexArray(0);
  glBlendFunc(GL_ONE, GL_ZERO);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

void Texture::RenderToScreenAsObject(const GraphicsObject& go,
                                     const SDLSurface& surface,
                                     const Rect& srcRect,
                                     const Rect& dstRect,
                                     int alpha) {
  // This all needs to be pushed up out of the rendering code and down into
  // either GraphicsObject or the individual GraphicsObjectData subclasses.

  // TODO(erg): Temporary hack while I wait to convert all of this machinery to
  // Rects.
  int xSrc1 = srcRect.x();
  int ySrc1 = srcRect.y();
  int xSrc2 = srcRect.x2();
  int ySrc2 = srcRect.y2();

  int fdx1 = dstRect.x(), fdy1 = dstRect.y(), fdx2 = dstRect.x2(),
      fdy2 = dstRect.y2();
  if (!filterCoords(xSrc1, ySrc1, xSrc2, ySrc2, fdx1, fdy1, fdx2, fdy2)) {
    return;
  }

  // Convert the pixel coordinates into [0,1) texture coordinates
  float thisx1 = float(xSrc1) / texture_width_;
  float thisy1 = float(ySrc1) / texture_height_;
  float thisx2 = float(xSrc2) / texture_width_;
  float thisy2 = float(ySrc2) / texture_height_;
  if (is_upside_down_) {
    thisy1 = float(logical_height_ - ySrc1) / texture_height_;
    thisy2 = float(logical_height_ - ySrc2) / texture_height_;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(fdx1, fdy1, 0));

  int width = fdx2 - fdx1;
  int height = fdy2 - fdy1;
  auto& param = go.Param();
  float x_rep = (width / 2.0f) + param.rep_origin_x();
  float y_rep = (height / 2.0f) + param.rep_origin_y();
  model = glm::translate(model, glm::vec3(x_rep, y_rep, 0.0f));
  model = glm::rotate(model, glm::radians(go.Param().rotation() / 10.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::translate(model, glm::vec3(-x_rep, -y_rep, 0.0f));

  // Make this so that when we have composite 1, we're doing a pure
  // additive blend, (ignoring the alpha channel?)
  switch (param.composite_mode()) {
    case 0:
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendEquation(GL_FUNC_ADD);
      break;
    case 1:
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glBlendEquation(GL_FUNC_ADD);
      break;
    case 2: {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
      break;
    }
    default: {
      std::ostringstream oss;
      oss << "Invalid composite_mode in render: " << param.composite_mode();
      throw SystemError(oss.str());
    }
  }

  auto toNDC = [w = s_screen_width, h = s_screen_height](glm::vec4 point) {
    const auto pt = glm::vec2(point) / point.w;
    return std::make_pair(2.0f * pt.x / w - 1.0f, 1.0f - (2.0f * pt.y / h));
  };
  const auto top_left = glm::vec4(0.0f, height, 0.0f, 1.0f);
  const auto bottom_right = glm::vec4(width, 0.0f, 0.0f, 1.0f);
  auto [dx1, dy1] = toNDC(model * top_left);
  auto [dx2, dy2] = toNDC(model * bottom_right);
  float vertices[] = {
      dx1, dy1, thisx1, thisy2,  // NOLINT
      dx2, dy1, thisx2, thisy2,  // NOLINT
      dx2, dy2, thisx2, thisy1,  // NOLINT
      dx1, dy2, thisx1, thisy1,  // NOLINT
  };
  unsigned int indices[] = {0, 1, 2, 0, 2, 3};
  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STREAM_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  auto shader = GetObjectShader();
  glUseProgram(shader->GetID());

  // bind texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  shader->SetUniform("image", 0);

  auto colour = param.colour();
  shader->SetUniform("colour", colour.r_float(), colour.g_float(),
                     colour.b_float(), colour.a_float());
  auto tint = param.tint();
  shader->SetUniform("tint", tint.r_float(), tint.g_float(), tint.b_float());
  shader->SetUniform("alpha", Normalize(alpha, 255));

  shader->SetUniform("mono", Normalize(param.mono(), 255));
  shader->SetUniform("invert", Normalize(param.invert(), 255));
  shader->SetUniform("light", Normalize(param.light(), 255));

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glUseProgram(0);
  glBindVertexArray(0);
  glBlendFunc(GL_ONE, GL_ZERO);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

static float our_round(float r) {
  return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

bool Texture::filterCoords(int& x1,
                           int& y1,
                           int& x2,
                           int& y2,
                           int& dx1,
                           int& dy1,
                           int& dx2,
                           int& dy2) {
  // POINT
  using std::max;
  using std::min;

  // Input: raw image coordinates
  // Output: false if this doesn't intersect with the texture piece we hold.
  //         true otherwise, and set the local coordinates
  int w1 = x2 - x1;
  int h1 = y2 - y1;

  // First thing we do is an intersection test to see if this input
  // range intersects the virtual range this Texture object holds.
  //
  /// @bug s/>/>=/?
  if (x1 + w1 >= x_offset_ && x1 < x_offset_ + logical_width_ &&
      y1 + h1 >= y_offset_ && y1 < y_offset_ + logical_height_) {
    // Do an intersection test in terms of the virtual coordinates
    int virX = max(x1, x_offset_);
    int virY = max(y1, y_offset_);
    int w = min(x1 + w1, x_offset_ + logical_width_) - max(x1, x_offset_);
    int h = min(y1 + h1, y_offset_ + logical_height_) - max(y1, y_offset_);

    // Adjust the destination coordinates
    int dx_width = dx2 - dx1;
    int dy_height = dy2 - dy1;
    float dx1Off = (virX - x1) / float(w1);
    dx1 = our_round(dx1 + (dx_width * dx1Off));
    float dx2Off = w / float(w1);
    dx2 = our_round(dx1 + (dx_width * dx2Off));
    float dy1Off = (virY - y1) / float(h1);
    dy1 = our_round(dy1 + (dy_height * dy1Off));
    float dy2Off = h / float(h1);
    dy2 = our_round(dy1 + (dy_height * dy2Off));

    // Output the source intersection in real (instead of
    // virtual) coordinates
    x1 = virX - x_offset_;
    x2 = x1 + w;
    y1 = virY - y_offset_;
    y2 = y1 + h;

    return true;
  }

  return false;
}
