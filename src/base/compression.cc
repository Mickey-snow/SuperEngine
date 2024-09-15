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

#include "compression.h"

#include "utilities/byte_reader.h"

#include <stdexcept>
#include <string>
#include <string_view>

std::string Decompress_lzss(std::string_view data) {
  if (data.empty())
    return "";

  if (data.size() < 8) {
    throw std::invalid_argument(
        "Data too small to contain a valid LZSS header");
  }

  ByteReader reader(data);
  int32_t arc_size = reader.PopAs<int32_t>(4);
  int32_t orig_size = reader.PopAs<int32_t>(4);

  if (arc_size != data.size())
    throw std::logic_error("File size mismatch");

  std::string result;
  bool should_repeat = true;
  while (should_repeat && result.size() < static_cast<size_t>(orig_size)) {
    uint8_t flags = reader.PopAs<uint8_t>(1);

    for (int bit = 0; bit < 8; ++bit) {
      if (result.size() >= static_cast<size_t>(orig_size)) {
        should_repeat = false;
        break;
      }

      if (flags & 1) {
        result += reader.PopAs<char>(1);
      } else {
        uint16_t chunk_data = reader.PopAs<uint16_t>(2);
        uint16_t chunk_size = 2 + (chunk_data & 0xf);
        size_t chunk_offset = result.size() - (chunk_data >> 4);

        for (size_t i = 0; i < chunk_size; ++i)
          result += result[chunk_offset + i];
      }

      flags >>= 1;
    }
  }

  if (result.size() != static_cast<size_t>(orig_size))
    throw std::runtime_error("Decompressed size does not match original size");
  return result;
}

std::string Decompress_lzss32(std::string_view data) { return ""; }
