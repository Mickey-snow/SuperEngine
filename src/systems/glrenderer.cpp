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

#include "systems/glrenderer.hpp"

#include "systems/gl_frame_buffer.hpp"
#include "systems/gl_utils.hpp"
#include "systems/gltexture.hpp"

#include <GL/glew.h>

void glRenderer::SetUp() {
  glEnable(GL_TEXTURE_2D);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glDepthFunc(GL_LEQUAL);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  ShowGLErrors();
}

void glRenderer::SetFrameBuffer(std::shared_ptr<glFrameBuffer> canvas) {
  canvas_ = canvas;
}

void glRenderer::ClearBuffer(RGBAColour color) {
  glBindFramebuffer(GL_FRAMEBUFFER, canvas_ ? canvas_->GetID() : 0);
  glClearColor(color.r_float(), color.g_float(), color.b_float(),
               color.a_float());
  glClear(GL_COLOR_BUFFER_BIT);
}
