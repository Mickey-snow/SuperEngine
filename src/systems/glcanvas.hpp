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

#pragma once

#include "core/rect.hpp"

#include <memory>
#include <optional>

class glTexture;
class glFrameBuffer;
class glRenderer;

class glCanvas {
 public:
  glCanvas(Size resolution,
           std::optional<Size> display_size = std::nullopt,
           std::optional<Point> origin = std::nullopt);

  void Use();

  std::shared_ptr<glFrameBuffer> GetBuffer() const;

  void Flush();

 private:
  Size resolution_, display_size_;
  Point origin_;
  std::shared_ptr<glFrameBuffer> frame_buf_;
  std::shared_ptr<glRenderer> renderer_;
};
