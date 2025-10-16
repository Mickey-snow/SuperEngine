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
//
// -----------------------------------------------------------------------

#pragma once

#include "core/rect.hpp"

#include <filesystem>
#include <memory>
#include <span>

class Album;
// TODO: Extract Surface abstraction from SDLSurface class
class SDLSurface;
using Surface = SDLSurface;

class IGraphicsBackend {
 public:
  virtual ~IGraphicsBackend() = default;

  virtual void InitSystem() = 0;
  virtual void QuitSystem() = 0;

  virtual std::shared_ptr<Surface> CreateSurfaceBGRA(Size size,
                                                     std::span<char> bgra,
                                                     bool is_alpha_mask) = 0;

  [[deprecated]]
  virtual std::shared_ptr<Surface> LoadSurface(
      const std::filesystem::path& path) = 0;
  virtual std::shared_ptr<Album> LoadAlbum(
      const std::filesystem::path& path) = 0;
};
