// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#pragma once

#include "core/grprect.hpp"

#include <ostream>
#include <string_view>
#include <vector>

/// Encodes an RGBA buffer into a binary PPM (P6) file.
/// @param os        Output stream.
/// @param width     Image width in pixels.
/// @param height    Image height in pixels.
/// @param rgba      Flat vector of size width*height*4, in R,G,B,A order.
/// @throws runtime_error on I/O error or invalid buffer size.
void saveRGBAasPPM(std::ostream& os,
                   int width,
                   int height,
                   const std::vector<char>& rgba);

class ImageDecoder {
 public:
  ImageDecoder(std::string_view sv);
  ~ImageDecoder() = default;

  bool ismask;
  int height, width;
  std::vector<GrpRect> region_table;
  std::vector<char> mem;
};
