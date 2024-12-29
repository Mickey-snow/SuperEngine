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

#include "systems/glcanvas.hpp"

#include "systems/gl_frame_buffer.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"
#include "systems/screen_canvas.hpp"

#include <cstring>

#include <GL/glew.h>

glCanvas::glCanvas(Size resolution, std::optional<Rect> viewport)
    : resolution_(resolution),
      viewport_(viewport.value_or(Rect(Point(0, 0), resolution))),
      frame_buf_(std::make_shared<glFrameBuffer>(
          std::make_shared<glTexture>(resolution))),
      renderer_(std::make_shared<glRenderer>()) {
  renderer_->ClearBuffer(frame_buf_, RGBAColour::Black());
}

void glCanvas::Use() {
  auto display_size = viewport_.size();
  glViewport(0, 0, display_size.width(), display_size.height());

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, (GLdouble)display_size.width(), (GLdouble)display_size.height(),
          0.0, 0.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  const auto aspect_ratio_w =
      static_cast<float>(display_size.width()) / resolution_.width();
  const auto aspect_ratio_h =
      static_cast<float>(display_size.height()) / resolution_.height();
  glTranslatef(viewport_.x() * aspect_ratio_w, viewport_.y() * aspect_ratio_h,
               0);
}

std::shared_ptr<glFrameBuffer> glCanvas::GetBuffer() const {
  return frame_buf_;
}

void glCanvas::Flush() {
  auto screen = std::make_shared<ScreenCanvas>(viewport_.size());
  const Rect src(Point(0, 0), resolution_);
  const Rect dst(Point(0, 0), viewport_.size());
  renderer_->Render({frame_buf_->GetTexture(), src}, {screen, dst});
}
