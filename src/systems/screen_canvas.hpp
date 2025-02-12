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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "core/rect.hpp"
#include "systems/gl_frame_buffer.hpp"

struct ScreenCanvas : public glFrameBuffer {
 public:
  ScreenCanvas(Size size) : size_(size) {}

  virtual unsigned int GetID() const override { return 0; }
  virtual Size GetSize() const override { return size_; }

  virtual std::shared_ptr<glTexture> GetTexture() const override;

  Size size_, display_size_;
};
