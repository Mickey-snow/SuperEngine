// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include "gtest/gtest.h"

#include "libreallive/bytecode.h"

#include <string>

using libreallive::ParsableToPrintableString;
using libreallive::PrintableToParsableString;

using std::string_literals::operator""s;

TEST(BytecodeFormattingTest, ParsableToPrintableString) {
  {
    unsigned char rawsrc[] = {0x28, 0x24, 0x06, 0x5B, 0x24, 0xFF, 0xE8, 0x03,
                              0x00, 0x00, 0x5D, 0x5C, 0x28, 0x24, 0xFF, 0x01,
                              0x00, 0x00, 0x00, 0x29, 0xD5, 0x01, 0x00, 0x00};
    std::string src(reinterpret_cast<const char*>(rawsrc),
                    sizeof(rawsrc) / sizeof(*rawsrc));
    EXPECT_EQ(
        ParsableToPrintableString(src),
        "( $ 06 [ $ ff e8 03 00 00 ] 5c ( $ ff 01 00 00 00 ) d5 01 00 00"s);
  }

  {
    unsigned char rawsrc[] = {0x24, 0xff, 0x28, 0x29, 0x5b, 0x5d};
    std::string src(reinterpret_cast<const char*>(rawsrc),
                    sizeof(rawsrc) / sizeof(*rawsrc));
    EXPECT_EQ(ParsableToPrintableString(src), "$ ff ( ) [ ]"s);
    // this really should be "$ ff 28 29 5b 5d"s
  }
}

TEST(BytecodeFormattingTest, PrintableToParsableString) {
  {
    std::string src = "( $ FF 01 10 00 00 )";
    char parsable_bytecode[] = {0x28, 0x24, 0xff, 0x01, 0x10, 0x00, 0x00, 0x29};
    EXPECT_STREQ(PrintableToParsableString(src).c_str(), parsable_bytecode)
        << "(4097)";
  }

  {
    std::string src =
        "( $ ff 00 00 00 00 $ 0b [ $ ff 00 00 00 00 ] 5c 00 $ ff 39 00 00 00 $ "
        "0b [ $ ff 01 00 00 00 ] 5c 00 $ ff 29 00 00 00 )";
    char parsable_bytecode[] = {
        0x28, 0x24, 0xff, 0x00, 0x00, 0x00, 0x00, 0x24, 0x0b, 0x5b, 0x24,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x24, 0xff, 0x39, 0x00,
        0x00, 0x00, 0x24, 0x0b, 0x5b, 0x24, 0xff, 0x01, 0x00, 0x00, 0x00,
        0x5d, 0x5c, 0x00, 0x24, 0xff, 0x29, 0x00, 0x00, 0x00};
    EXPECT_STREQ(PrintableToParsableString(src).c_str(), parsable_bytecode)
        << "(0, intL[0] + 57, intL[1] + 41)";
  }
}

// In later games, you found newline metadata inside special parameters(?) Make
// sure that the expression parser can deal with that.
TEST(ExpressionParserTest, ParseWithNewlineInIt) {
  string parsable = libreallive::PrintableToParsableString(
      "0a 77 02 61 37 61 00 ( $ ff 29 00 00 00 5c 02 $ ff 8d 01 00 00 "
      "$ ff ff 00 00 00 )");

  libreallive::Expression piece;
  EXPECT_NO_THROW({
    const char* start = parsable.c_str();
    piece = libreallive::GetData(start);
  });

  ASSERT_TRUE(piece->IsSpecialParameter());
}

TEST(ParserTest, ParseQuotedEnglishString) {
  string s = "\"Say \\\"Hello.\\\"\"";

  ASSERT_EQ(16, libreallive::NextString(s.c_str()));
}
