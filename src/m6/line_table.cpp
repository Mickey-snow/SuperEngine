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

#include "m6/line_table.hpp"

#include <algorithm>

namespace m6 {

LineTable::LineTable(std::string_view src) : src_(src) {
  PrecomputeLineStarts();
}

std::tuple<size_t, size_t> LineTable::Find(size_t offset) const {
  // Clamp the offset within the valid range.
  offset = std::clamp(offset, static_cast<size_t>(0), src_.size());

  auto it = std::upper_bound(lineStarts_.cbegin(), lineStarts_.cend(), offset);
  if (it != lineStarts_.cbegin())
    --it;  // Step back one iterator to get the line whose start <= offset

  size_t line = std::distance(lineStarts_.cbegin(), it);
  size_t column = offset - lineStarts_[line];
  return std::make_tuple(line, column);
}

std::string_view LineTable::LineText(size_t lineIndex) const {
  if (lineIndex >= lineStarts_.size() - 1)
    return {};

  size_t start = lineStarts_[lineIndex];
  size_t end = lineStarts_[lineIndex + 1];  // next line start
  if (end > 0 && src_[end - 1] == '\n')
    --end;  // remove trailing newline if it exists
  return src_.substr(start, end - start);
}

size_t LineTable::LineCount() const {
  // The last element is a sentinel “end of file” line,
  // so the actual number of lines is lineStarts_.size() - 1.
  return (lineStarts_.empty()) ? 0 : (lineStarts_.size() - 1);
}

void LineTable::PrecomputeLineStarts() {
  lineStarts_.clear();
  lineStarts_.push_back(0);
  for (size_t i = 0; i < src_.size(); ++i) {
    if (src_[i] == '\n') {
      lineStarts_.push_back(i + 1);
    }
  }
  // Sentinel for "end of file"
  lineStarts_.push_back(src_.size() + 1);
}

}  // namespace m6
