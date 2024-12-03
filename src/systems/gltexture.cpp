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

#include "systems/gltexture.hpp"
#include "systems/gl_utils.hpp"

#include <GL/gl.h>

glTexture::glTexture(Size size, uint8_t* data) : size_(size) {
  glGenTextures(1, &id_);
  glBindTexture(GL_TEXTURE_2D, id_);
  ShowGLErrors();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size_.width(), size_.height(), 0,
               GL_RGBA, GL_UNSIGNED_BYTE, data);
  ShowGLErrors();
}

glTexture::~glTexture() { glDeleteTextures(1, &id_); }

unsigned int glTexture::GetID() const { return id_; }

std::vector<uint8_t> glTexture::Dump(Rect region) {
  std::vector<uint8_t> data(region.width() * region.height() * 4);
  glBindTexture(GL_TEXTURE_2D, id_);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
  glBindTexture(GL_TEXTURE_2D, 0);
  return data;
}
