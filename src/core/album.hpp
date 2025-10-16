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

#pragma once

class SDLSurface;
using Surface = SDLSurface;

#include "core/grprect.hpp"

#include <memory>
#include <vector>

struct Image {
  std::shared_ptr<Surface> surface;
  GrpRect region;
};

class Album {
 public:
  Album(std::shared_ptr<Surface> surface,
        std::vector<GrpRect> region_table = {});

  [[nodiscard]] inline std::shared_ptr<Surface> GetSurface() const {
    return surface_;
  }
  [[nodiscard]] inline std::size_t GetNumPatterns() const {
    return region_table_.size();
  }
  [[nodiscard]] inline std::vector<GrpRect> const& GetPatternTable() const {
    return region_table_;
  }
  [[nodiscard]] inline GrpRect GetPattern(int no) const {
    return region_table_.at(no);
  }
  [[nodiscard]] Image GetImage(int pattern_no) const;

 private:
  std::shared_ptr<Surface> surface_;
  std::vector<GrpRect> region_table_;
};
