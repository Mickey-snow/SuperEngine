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
#include "m6/source_buffer.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace m6 {

ErrorFormatter::ErrorFormatter() = default;

ErrorFormatter& ErrorFormatter::Pushline(const std::string& msg) {
  oss << msg << '\n';
  return *this;
}

ErrorFormatter& ErrorFormatter::Highlight(const SourceLocation& loc,
                                          const std::string& msg) {
  const auto sb = loc.src;
  const std::size_t size = sb->Size();

  auto begin = loc.begin_offset;
  auto end = loc.end_offset;

  if (begin > size)
    begin = size;
  if (end > size)
    end = size;

  // Are we highlighting an insertion point rather than a range?
  const bool isInsertion = (begin == end);

  auto [lineBegin, colBegin] = sb->GetLineColumn(begin);
  auto [lineEnd, colEnd] = sb->GetLineColumn(end);

  if (!msg.empty())
    oss << "At file '" << sb->GetFile() << "' " << msg << '\n';
  const auto digitLen =
      std::to_string(std::max(lineBegin, lineEnd) + 1).length();
  const size_t prefLen = digitLen + 2;  // "NN│ "

  // Only one line in the insertion case, but loop covers both single- and
  // multi-line
  for (size_t lineIdx = lineBegin; lineIdx <= lineEnd; ++lineIdx) {
    auto lineText = sb->GetLine(lineIdx);

    // 1) print the source line
    oss << std::setw(digitLen) << std::left << (lineIdx + 1) << "│ " << lineText
        << '\n';

    // 2) compute where on this line we’d normally highlight
    size_t highlightBeginCol = 0;
    size_t highlightEndCol = lineText.size();
    if (!isInsertion) {
      if (lineIdx == lineBegin)
        highlightBeginCol = colBegin;
      if (lineIdx == lineEnd)
        highlightEndCol = colEnd;
      highlightBeginCol = std::min(highlightBeginCol, lineText.size());
      highlightEndCol = std::min(highlightEndCol, lineText.size());
    }

    // 3) print the caret line
    if (isInsertion) {
      // single caret at insertion point
      size_t pos = std::min(colBegin, lineText.size());
      // pad up to the caret position + 1 so we can place '^'
      std::string caretLine(prefLen + pos + 1, ' ');
      caretLine[prefLen + pos] = '^';
      oss << caretLine << '\n';
    } else if (highlightBeginCol < highlightEndCol) {
      // a continuous span of carets over [beginCol, endCol)
      std::string caretLine(prefLen + lineText.size(), ' ');
      std::fill(caretLine.begin() + prefLen + highlightBeginCol,
                caretLine.begin() + prefLen + highlightEndCol, '^');
      oss << caretLine << '\n';
    }
  }

  return *this;
}

std::string ErrorFormatter::Str() {
  std::string result = oss.str();
  oss.str("");
  oss.clear();
  return result;
}

}  // namespace m6
