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

#ifndef BASE_AVDEC_IMAGE_DECODER_H_
#define BASE_AVDEC_IMAGE_DECODER_H_

#include "xclannad/file.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

class ImageDecoder {
 public:
  ImageDecoder(std::string_view sv) {
    std::unique_ptr<GRPCONV> conv(
        GRPCONV::AssignConverter(sv.data(), sv.size(), "???"));
    if (!conv)
      throw std::runtime_error("Failure at creating GRPCONV.");

    ismask = conv->IsMask();
    height = conv->Height();
    width = conv->Width();
    region_table = conv->region_table;
    mem.resize(width * height * 4);
    if (!conv->Read(mem.data()))
      throw std::runtime_error("Xclannad converter failed.");
  }
  ~ImageDecoder() = default;

  bool ismask;
  int height, width;
  std::vector<GRPCONV::REGION> region_table;
  std::vector<char> mem;
};

#endif
