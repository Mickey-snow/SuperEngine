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

#include "m6/error_formatter.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace m6 {

ErrorFormatter::ErrorFormatter(std::string_view src) : src_(src), index_(src) {}

ErrorFormatter& ErrorFormatter::Pushline(const std::string& msg) {
  oss << msg << '\n';
  return *this;
}

ErrorFormatter& ErrorFormatter::Highlight(size_t begin,
                                          size_t end,
                                          const std::string& msg) {
  if (begin > src_.size())
    begin = src_.size();
  if (end > src_.size())
    end = src_.size();

  auto [lineBegin, colBegin] = index_.Find(begin);
  auto [lineEnd, colEnd] = index_.Find(end);

  oss << msg << '\n';
  const auto digitLen = std::to_string(std::max(lineEnd, lineBegin)).length();

  // For each line from lineBegin to lineEnd, print the line plus caret(s)
  for (size_t lineIdx = lineBegin; lineIdx <= lineEnd; ++lineIdx) {
    auto lineText = index_.LineText(lineIdx);

    oss << std::setw(digitLen) << std::left << lineIdx << "│ ";
    oss << lineText << "\n";

    // Determine which portion of this line to highlight.
    size_t highlightBeginCol = 0;
    size_t highlightEndCol = lineText.size();

    if (lineIdx == lineBegin) {
      highlightBeginCol = colBegin;
    }
    if (lineIdx == lineEnd) {
      highlightEndCol = colEnd;
    }

    // Clamp the highlight range to the line length.
    highlightBeginCol = std::min(highlightBeginCol, lineText.size());
    highlightEndCol = std::min(highlightEndCol, lineText.size());

    // Only print a caret line if there is a positive highlight length.
    if (highlightBeginCol < highlightEndCol) {
      std::string caretLine(lineText.size(), ' ');
      std::fill(caretLine.begin() + highlightBeginCol,
                caretLine.begin() + highlightEndCol, '^');
      oss << caretLine << "\n";
    }
  }

  return *this;
}

std::string ErrorFormatter::Flush() {
  std::string result = oss.str();
  oss.str("");
  oss.clear();
  return result;
}

}  // namespace m6
