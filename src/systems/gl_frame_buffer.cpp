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

#include "systems/gl_frame_buffer.hpp"

#include "base/rect.hpp"
#include "systems/gl_utils.hpp"
#include "systems/gltexture.hpp"

#include <GL/glew.h>

#include <stdexcept>

glFrameBuffer::glFrameBuffer(std::shared_ptr<glTexture> texture)
    : texture_(texture) {
  glGenFramebuffers(1, &id_);
  glBindFramebuffer(GL_FRAMEBUFFER, id_);
  glBindTexture(GL_TEXTURE_2D, texture->GetID());
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture->GetID(), 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    throw std::runtime_error("glFrameBuffer: Framebuffer not complete!");
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  ShowGLErrors();
}

glFrameBuffer::~glFrameBuffer() { glDeleteFramebuffers(1, &id_); }

unsigned int glFrameBuffer::GetID() const { return id_; }

Size glFrameBuffer::GetSize() const { return texture_->GetSize(); }
