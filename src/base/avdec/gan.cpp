// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include "base/avdec/gan.hpp"

#include "libreallive/alldefs.hpp"

#include <stdexcept>
#include <string>

using libreallive::read_i32;

GanDecoder::GanDecoder(std::vector<char> gan_data) {
  const char* data = gan_data.data();

  int a = read_i32(data);
  int b = read_i32(data + 0x04);
  int c = read_i32(data + 0x08);

  if (a != 10000 || b != 10000 || c != 10100) {
    ThrowBadFormat("Incorrect GAN file magic.");
  }

  int file_name_length = read_i32(data + 0xc);
  raw_file_name = std::string(data + 0x10, file_name_length);
  if (!raw_file_name.empty() && raw_file_name.back() == '\0')
    raw_file_name.pop_back();

  // Move data pointer past the filename
  data = data + 0x10 + file_name_length - 1;
  if (*data != 0) {
    ThrowBadFormat("Incorrect filename length in GAN header");
  }
  data++;

  int twenty_thousand = read_i32(data);
  if (twenty_thousand != 20000) {
    ThrowBadFormat("Expected start of GAN data section");
  }
  data += 4;

  int number_of_sets = read_i32(data);
  data += 4;

  for (int i = 0; i < number_of_sets; ++i) {
    int start_of_ganset = read_i32(data);
    if (start_of_ganset != 0x7530) {
      ThrowBadFormat("Expected start of GAN set");
    }

    data += 4;
    int frame_count = read_i32(data);
    if (frame_count < 0) {
      ThrowBadFormat("Expected animation to contain at least one frame");
    }
    data += 4;

    std::vector<Frame> animation_set;
    animation_set.reserve(frame_count);
    for (int j = 0; j < frame_count; ++j) {
      animation_set.push_back(ReadSetFrame(data));
    }

    animation_sets.emplace_back(std::move(animation_set));
  }
}

void GanDecoder::ThrowBadFormat(const std::string& msg) {
  throw std::runtime_error("GanDecoder: " + msg);
}

GanDecoder::Frame GanDecoder::ReadSetFrame(const char*& data) {
  Frame frame{};

  int tag = read_i32(data);
  data += 4;
  while (tag != 999999) {
    int value = read_i32(data);
    data += 4;

    switch (tag) {
      case 30100:
        frame.pattern = value;
        break;
      case 30101:
        frame.x = value;
        break;
      case 30102:
        frame.y = value;
        break;
      case 30103:
        frame.time = value;
        break;
      case 30104:
        frame.alpha = value;
        break;
      case 30105:
        frame.other = value;
        break;
      default:
        throw std::runtime_error("GanDecoder: Unknown GAN frame tag " +
                                 std::to_string(tag));
    }

    tag = read_i32(data);
    data += 4;
  }

  return frame;
}
