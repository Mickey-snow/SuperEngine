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

#include "base/rect.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <vector>

class RGBAColour;

// adapter to opengl texture, encapsulate the logic to read from and write to
// the texture, automatically translate between sdl and opengl coordinate
// systems.
class glTexture {
 private:
  auto Flip_y(Size size, std::input_iterator auto it) -> std::vector<uint8_t> {
    const auto h = size.height();
    const auto w = size.width() * 4;
    std::vector<uint8_t> result(h * w);
    for (int y = h - 1; y >= 0; --y)
      for (int x = 0; x < w; ++x)
        result[y * w + x] = *it++;
    return result;
  }
  // Flip the region's coordinates from top-left to bottom-left or vice versa.
  auto Flip_y(Rect region) -> Rect {
    return Rect(
        Point(region.x(), size_.height() - region.y() - region.height()),
        Size(region.width(), region.height()));
  }

 public:
  glTexture(Size size, uint8_t* data = nullptr);
  glTexture(Size size, std::ranges::input_range auto&& range) {
    auto data = Flip_y(size, std::ranges::cbegin(range));
    Init(size, data.data());
  }
  ~glTexture();

  unsigned int GetID() const;
  Size GetSize() const;

 private:
  void Init(Size size, uint8_t* data);

 public:
  void Write(Rect region, uint32_t format, uint32_t type, const void* data);
  void Write(Rect region, std::vector<uint8_t> data);

  std::vector<RGBAColour> Dump(std::optional<Rect> region = std::nullopt);

 private:
  unsigned int id_;
  Size size_;
};
