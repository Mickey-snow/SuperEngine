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

// The code in this file has been modified from the file anm.cc in
// Jagarl's xkanon project.

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

class AnmDecoder {
 public:
  // Constructor
  explicit AnmDecoder(std::vector<char> anm_data);

  // Frame struct
  struct Frame {
    int src_x1, src_y1;
    int src_x2, src_y2;
    int dest_x, dest_y;
    int time;
  };

  // Public members
  std::string raw_file_name;
  std::vector<Frame> frames;
  std::vector<std::vector<int>> framelist_;
  std::vector<std::vector<int>> animation_set_;

  void FixAxis(Frame& frame, int width, int height);

 private:
  // Helper functions
  bool TestFileMagic(const std::vector<char>& anm_data);
  void ReadIntegerList(const char* start,
                       int offset,
                       int iterations,
                       std::vector<std::vector<int>>& dest);
};
