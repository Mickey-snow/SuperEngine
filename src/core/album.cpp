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
// -----------------------------------------------------------------------

#include "core/album.hpp"

#include "systems/sdl_surface.hpp"

Album::Album(std::shared_ptr<Surface> surface,
             std::vector<GrpRect> region_table)
    : surface_(surface), region_table_(std::move(region_table)) {
  if (region_table_.empty()) {
    GrpRect region;
    region.rect = surface->GetRect();
    region_table_.emplace_back(region);
  }
}

Image Album::GetImage(int pattern_no) const {
  return Image{.surface = surface_, .region = GetPattern(pattern_no)};
}
