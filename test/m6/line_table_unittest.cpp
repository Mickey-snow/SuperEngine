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

#include "m6/line_table.hpp"

#include <string>

using m6::LineTable;

TEST(LineTableTest, EmptyInput) {
  std::string_view text = "";
  LineTable index(text);

  // Only 1 line
  EXPECT_EQ(index.LineCount(), 1u);

  // Find() with offset 0 should clamp to 0
  auto [line, col] = index.Find(0);
  EXPECT_EQ(line, 0u);
  EXPECT_EQ(col, 0u);

  // line text is empty
  EXPECT_TRUE(index.LineText(0).empty());
}

TEST(LineTableTest, SingleLineNoNewline) {
  std::string_view text = "Hello world!";
  LineTable index(text);

  // Only 1 line
  EXPECT_EQ(index.LineCount(), 1u);

  // Check line text
  EXPECT_EQ(index.LineText(0), text);

  // Test offsets
  {
    auto [line, col] = index.Find(0);
    EXPECT_EQ(line, 0u);
    EXPECT_EQ(col, 0u);
  }
  {
    auto [line, col] = index.Find(5);
    EXPECT_EQ(line, 0u);
    EXPECT_EQ(col, 5u);
  }
  {
    // Past the end -> should clamp
    auto [line, col] = index.Find(text.size() + 10);
    EXPECT_EQ(line, 0u);
    EXPECT_EQ(col, text.size());
  }
}

TEST(LineTableTest, SingleLineWithNewline) {
  // A single line plus a trailing newline.
  std::string_view text = "Hello world!\n";
  LineTable index(text);

  // line 0: "Hello world!"
  // line 1: "" (empty line, because there's a trailing newline)
  EXPECT_EQ(index.LineCount(), 2u);

  EXPECT_EQ(index.LineText(0), "Hello world!");
  EXPECT_EQ(index.LineText(1), "");

  // Offsets
  {
    auto [line, col] = index.Find(0);
    EXPECT_EQ(line, 0u);
    EXPECT_EQ(col, 0u);
  }
  {
    auto [line, col] = index.Find(text.size() - 1);  // offset at newline char
    EXPECT_EQ(line, 0u);
    EXPECT_EQ(col, 12u);
  }
  {
    // Past the end
    auto [line, col] = index.Find(text.size() + 5);
    EXPECT_EQ(line, 1u);
    EXPECT_EQ(col, 0u);
  }
}

TEST(LineTableTest, MultipleLines) {
  //   Line 0: "alpha"
  //   Line 1: "beta"
  //   Line 2: "gamma"
  std::string_view text = "alpha\nbeta\ngamma";
  LineTable index(text);

  EXPECT_EQ(index.LineCount(), 3u);

  EXPECT_EQ(index.LineText(0), "alpha");
  EXPECT_EQ(index.LineText(1), "beta");
  EXPECT_EQ(index.LineText(2), "gamma");

  EXPECT_EQ(index.Find(0), std::make_tuple(0u, 0u));
  EXPECT_EQ(index.Find(4), std::make_tuple(0u, 4u));
  EXPECT_EQ(index.Find(5), std::make_tuple(0u, 5u));
  EXPECT_EQ(index.Find(6), std::make_tuple(1u, 0u));
  EXPECT_EQ(index.Find(10), std::make_tuple(1u, 4u));
  EXPECT_EQ(index.Find(11), std::make_tuple(2u, 0u));
  EXPECT_EQ(index.Find(15), std::make_tuple(2u, 4u));

  // Past end
  auto [line, col] = index.Find(999);
  EXPECT_EQ(line, 2u);
  EXPECT_EQ(col, 5u);
}

TEST(LineTableTest, MultipleLinesTrailingNewline) {
  std::string_view text = "line1\nline2\nline3\n";
  LineTable index(text);

  EXPECT_EQ(index.LineCount(), 4u);

  EXPECT_EQ(index.LineText(0), "line1");
  EXPECT_EQ(index.LineText(1), "line2");
  EXPECT_EQ(index.LineText(2), "line3");
  EXPECT_EQ(index.LineText(3), "");

  EXPECT_EQ(index.Find(0), std::make_tuple(0u, 0u));
  EXPECT_EQ(index.Find(4), std::make_tuple(0u, 4u));
  EXPECT_EQ(index.Find(5), std::make_tuple(0u, 5u));
  EXPECT_EQ(index.Find(10), std::make_tuple(1u, 4u));
  EXPECT_EQ(index.Find(11), std::make_tuple(1u, 5u));
  EXPECT_EQ(index.Find(16), std::make_tuple(2u, 4u));
  EXPECT_EQ(index.Find(17), std::make_tuple(2u, 5u));
}

TEST(LineTableTest, OutOfRangeLineText) {
  std::string_view text = "abc\ndef";
  LineTable index(text);

  EXPECT_EQ(index.LineCount(), 2u);

  EXPECT_EQ(index.LineText(0), "abc");
  EXPECT_EQ(index.LineText(1), "def");

  EXPECT_TRUE(index.LineText(2).empty());
  EXPECT_TRUE(index.LineText(999).empty());
}

TEST(LineTableTest, StressTestLongString) {
  std::string largeText;
  for (int i = 0; i < 1000; ++i) {
    largeText += "Line" + std::to_string(i) + "\n";
  }

  LineTable index(largeText);

  EXPECT_EQ(index.LineCount(), 1001u);

  EXPECT_EQ(index.LineText(0), "Line0");
  EXPECT_EQ(index.LineText(500), "Line500");
  EXPECT_EQ(index.LineText(999), "Line999");
  EXPECT_EQ(index.LineText(1000), "");

  auto [line, col] = index.Find(largeText.size() - 1);
  EXPECT_EQ(line, 999u);
  EXPECT_EQ(col, 7u);
}
