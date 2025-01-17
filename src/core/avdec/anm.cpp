// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
// Copyright (C) 2000 Kazunori Ueno(JAGARL) <jagarl@creator.club.ne.jp>
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

#include "core/avdec/anm.hpp"

#include "libreallive/alldefs.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

using libreallive::read_i32;

AnmDecoder::AnmDecoder(std::vector<char> anm_data) {
  // Ensure the file’s magic is correct
  if (TestFileMagic(anm_data)) {
    throw std::runtime_error(
        "AnmDecoder: Data does not appear to be in ANM format.");
  }

  // Parse basic header fields
  const char* data = anm_data.data();
  int frames_len = read_i32(data + 0x8c);
  int framelist_len = read_i32(data + 0x90);
  int animation_set_len = read_i32(data + 0x94);

  // Sanity check
  if (animation_set_len < 0) {
    throw std::runtime_error(
        "AnmDecoder: Impossible value for animation_set_len in ANM file.");
  }

  // The name of the raw file (image) is stored at offset 0x1c
  raw_file_name = data + 0x1c;

  // Read frame data
  const char* buf = data + 0xb8;
  for (int i = 0; i < frames_len; ++i) {
    Frame f;
    f.src_x1 = read_i32(buf);
    f.src_y1 = read_i32(buf + 4);
    f.src_x2 = read_i32(buf + 8);
    f.src_y2 = read_i32(buf + 12);
    f.dest_x = read_i32(buf + 16);
    f.dest_y = read_i32(buf + 20);
    f.time = read_i32(buf + 0x38);

    frames.push_back(f);
    buf += 0x60;  // Move to the next frame record
  }

  // Read framelist and animation sets
  ReadIntegerList(data + 0xb8 + frames_len * 0x60, 0x68, framelist_len,
                  framelist_);

  ReadIntegerList(data + 0xb8 + frames_len * 0x60 + framelist_len * 0x68, 0x78,
                  animation_set_len, animation_set_);
}

bool AnmDecoder::TestFileMagic(const std::vector<char>& anm_data) {
  static const int ANM_MAGIC_SIZE = 12;
  static const char ANM_MAGIC[ANM_MAGIC_SIZE] = {'A', 'N', 'M', '3', '2', 0,
                                                 0,   0,   0,   1,   0,   0};

  if (anm_data.size() < ANM_MAGIC_SIZE) {
    return false;
  }

  for (int i = 0; i < ANM_MAGIC_SIZE; ++i) {
    if (ANM_MAGIC[i] != anm_data[i]) {
      return false;
    }
  }
  return true;
}

void AnmDecoder::ReadIntegerList(const char* start,
                                 int offset,
                                 int iterations,
                                 std::vector<std::vector<int>>& dest) {
  for (int i = 0; i < iterations; ++i) {
    int list_length = read_i32(start + 4);
    const char* tmpbuf = start + 8;

    // Build one row of integers
    std::vector<int> intlist;
    intlist.reserve(list_length);

    for (int j = 0; j < list_length; ++j) {
      intlist.push_back(read_i32(tmpbuf));
      tmpbuf += 4;
    }

    dest.push_back(intlist);
    start += offset;  // Move to the next list block
  }
}

void AnmDecoder::FixAxis(Frame& frame, int width, int height) {
  // Ensure correct ordering
  if (frame.src_x1 > frame.src_x2) {
    std::swap(frame.src_x1, frame.src_x2);
  }
  if (frame.src_y1 > frame.src_y2) {
    std::swap(frame.src_y1, frame.src_y2);
  }

  // Clamp right edge
  if (frame.dest_x + (frame.src_x2 - frame.src_x1 + 1) > width) {
    frame.src_x2 = frame.src_x1 + (width - frame.dest_x);
  }

  // NOTE: the original code uses 'width' for both axes. Adjust if that's a
  // typo:
  if (frame.dest_y + (frame.src_y2 - frame.src_y1 + 1) > height) {
    frame.src_y2 = frame.src_y1 + (height - frame.dest_y);
  }
}
