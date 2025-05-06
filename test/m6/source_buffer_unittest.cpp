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

#include <gtest/gtest.h>

#include "m6/source_buffer.hpp"
#include "m6/source_location.hpp"

namespace m6test {

using namespace m6;

// A simple helper to split lines for comparison
static std::vector<std::string_view> split_lines(std::string_view s) {
  std::vector<std::string_view> lines;
  size_t start = 0;
  while (true) {
    auto pos = s.find('\n', start);
    if (pos == std::string_view::npos) {
      lines.push_back(s.substr(start));
      break;
    }
    lines.push_back(s.substr(start, pos - start));
    start = pos + 1;
  }
  return lines;
}

TEST(SourceBuffer_Typical, CreateAndBasicAccess) {
  const std::string code = "first line\nsecond line\nthird";
  const std::string filename = "example.txt";

  auto buf = SourceBuffer::Create(code, filename);
  // initial use_count == 1
  EXPECT_EQ(buf.use_count(), 1);

  // GetStr / GetFile / GetView
  EXPECT_EQ(buf->GetStr(), code);
  EXPECT_EQ(buf->GetFile(), filename);
  EXPECT_EQ(buf->GetView(), code);

  // split into lines and compare with GetLine
  auto expected = split_lines(buf->GetView());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(buf->GetLine(i), expected[i]);
  }

  // GetLineColumn:
  // offset 0 -> line 0, col 0
  auto [l0, c0] = buf->GetLineColumn(0);
  EXPECT_EQ(l0, 0u);
  EXPECT_EQ(c0, 0u);

  // offset at first '\n' -> points to newline char
  // we treat newline as end of line 0, column == length of line 0
  EXPECT_EQ(buf->GetView()[expected[0].size()], '\n');
  auto [ln, cn] = buf->GetLineColumn(expected[0].size());
  EXPECT_EQ(ln, 0u);
  EXPECT_EQ(cn, expected[0].size());

  // offset just after newline -> start of line 1
  auto [l1, c1] = buf->GetLineColumn(expected[0].size() + 1);
  EXPECT_EQ(l1, 1u);
  EXPECT_EQ(c1, 0u);

  // GetReference and GetReferenceAt
  auto loc = buf->GetReference(5, 15);
  // new use_count == 2 (one in buf, one in loc.src)
  EXPECT_EQ(buf.use_count(), 2);
  EXPECT_EQ(loc.begin_offset, 5u);
  EXPECT_EQ(loc.end_offset, 15u);
  EXPECT_EQ(loc.src.get(), buf.get());

  auto loc2 = buf->GetReferenceAt(7);
  buf = nullptr;
  EXPECT_EQ(loc2.src.use_count(), 2);
  EXPECT_EQ(loc2.begin_offset, 7u);
  EXPECT_EQ(loc2.end_offset, 7u);
  EXPECT_EQ(loc2.src.get(), loc.src.get());
}

}  // namespace m6test
