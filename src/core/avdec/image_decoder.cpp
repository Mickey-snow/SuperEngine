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

#include "core/avdec/image_decoder.hpp"

#include "xclannad/file.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>

void saveRGBAasPPM(std::ostream& os,
                   int width,
                   int height,
                   const std::vector<char>& rgba) {
  // Check that we have exactly width*height*4 bytes
  if (rgba.size() != static_cast<size_t>(width) * height * 4) {
    throw std::runtime_error("RGBA buffer has wrong size");
  }

  if (!os) {
    throw std::runtime_error("Failed to open output file");
  }

  // PPM header for binary P6 format
  os << "P6\n"
     << width << " " << height << "\n"
     << "255\n";

  // Write pixel data: for each pixel, output R, G, B (skip A)
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
      os.put(rgba[idx + 0]);
      os.put(rgba[idx + 1]);
      os.put(rgba[idx + 2]);
    }
  }

  if (!os) {
    throw std::runtime_error("Error occurred while writing pixel data");
  }
}

// ------------------------------------------------------------------------------

ImageDecoder::ImageDecoder(std::string_view sv) {
  std::unique_ptr<IConverter> conv =
      IConverter::CreateConverter(sv.data(), sv.size());
  if (!conv)
    throw std::runtime_error("Failure at creating GRPCONV.");

  ismask = conv->IsMask();
  height = conv->Height();
  width = conv->Width();

  if (!conv->region_table.empty()) {
    region_table.reserve(conv->region_table.size());
    auto xclannadRegionConverter = [](const IConverter::Region& region) {
      return GrpRect{.rect = Rect(Point(region.x1, region.y1),
                                  Point(region.x2 + 1, region.y2 + 1)),
                     .originX = region.origin_x,
                     .originY = region.origin_y};
    };

    std::transform(conv->region_table.cbegin(), conv->region_table.cend(),
                   std::back_inserter(region_table), xclannadRegionConverter);
  }

  mem.resize(width * height * 4);
  if (!conv->Read(mem.data()))
    throw std::runtime_error("Xclannad converter failed.");
}
