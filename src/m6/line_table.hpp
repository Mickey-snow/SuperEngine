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
//
// -----------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace m6 {

class LineTable {
 public:
  /// Construct from the source text
  explicit LineTable(std::string_view src);

  /// Returns the line and column number that contains the given offset.
  /// Lines and column numbers are zero-based.
  std::tuple<size_t, size_t> Find(size_t offset) const;

  /// Returns the substring for the full line of `lineIndex`
  std::string_view LineText(size_t lineIndex) const;

  /// Returns the total number of lines
  size_t LineCount() const;

 private:
  /// Helper function to precompute the starting index of each line.
  void PrecomputeLineStarts();

  std::string_view src_;
  std::vector<size_t> lineStarts_;
};

}  // namespace m6
