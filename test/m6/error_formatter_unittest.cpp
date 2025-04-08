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

#include "m6/error_formatter.hpp"
#include "util.hpp"

#include <string>
#include <string_view>

namespace m6test {

using namespace m6;

TEST(ErrorFormatterTest, HighlightRegion) {
  constexpr std::string_view src = "a+b-c";
  ErrorFormatter formatter(src);
  formatter.Highlight(2, 5, "msg1");
  EXPECT_TXTEQ(formatter.Str(), R"(
msg1
1│ a+b-c
     ^^^
)");
}

TEST(ErrorFormatterTest, HighlightMultiline) {
  std::string src;
  for (int i = 0; i < 10; ++i)
    src += "a+" + std::to_string(i) + '\n';
  ErrorFormatter formatter(src);
  formatter.Highlight(34, 38, "msg2");
  EXPECT_TXTEQ(formatter.Str(), R"(
msg2
9 │ a+8
      ^
10│ a+9
    ^^
)");
}

TEST(ErrorFormatterTest, HighlightEndOfLine) {
  constexpr std::string_view src = "a+b\na+c";
  ErrorFormatter formatter(src);
  formatter.Highlight(3, 3, "Missing ; here");
  formatter.Highlight(7, 7, "Missing ; here");
  EXPECT_TXTEQ(formatter.Str(), R"(
Missing ; here
1│ a+b
      ^
Missing ; here
2│ a+c
      ^
)");
}

TEST(ErrorFormatterTest, HighlightAt) {
  constexpr std::string_view src = "a+b\na+c";
  ErrorFormatter formatter(src);
  formatter.Highlight(1, 1, "at +");
  formatter.Highlight(4, 4, "at a");

  EXPECT_TXTEQ(formatter.Str(), R"(
at +
1│ a+b
    ^
at a
2│ a+c
   ^
)");
}

}  // namespace m6test
