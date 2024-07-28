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

#include "encodings/cp932.h"
#include "libreallive/parser.h"

#include <string>

using namespace libreallive;

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
  std::string parsable = libreallive::PrintableToParsableString(
      "0a 77 02 61 37 61 00 ( $ ff 29 00 00 00 5c 02 $ ff 8d 01 00 00 "
      "$ ff ff 00 00 00 )");

  libreallive::Expression piece;
  EXPECT_NO_THROW({
    const char* start = parsable.c_str();
    piece = libreallive::ExpressionParser::GetData(start);
  });

  ASSERT_TRUE(piece->IsSpecialParameter());
}

// -----------------------------------------------------------------------
// CommaParserTest
// -----------------------------------------------------------------------

TEST(CommaParserTest, ParseCommaElement) {
  Parser parser;
  std::string parsable = PrintableToParsableString("00");
  auto parsed = parser.ParseBytecode(parsable);
  if (auto commaElement = dynamic_cast<CommaElement*>(parsed)) {
    auto repr = commaElement->GetSourceRepresentation(nullptr);
    EXPECT_EQ(repr, "<CommaElement>"s);
    std::ostringstream oss;
    parsed->PrintSourceRepresentation(nullptr, oss);
    EXPECT_EQ(oss.str(), "<CommaElement>"s);
  } else {
    ADD_FAILURE()
        << "Parser failed to produce CommaElement object from '<CommaElement>'";
  }
  delete parsed;
}

// -----------------------------------------------------------------------
// TextoutParserTest
// -----------------------------------------------------------------------

TEST(TextoutParserTest, ParseCp932Text) {
  Parser parser;
  std::string parsable = PrintableToParsableString(
      "81 79 90 ba 81 7a 81 75 82 ab 82 e5 81 5b 82 b7 82 af 82 aa 8b 41 82 c1 "
      "82 c4 82 ab 82 bd 82 bc 81 5b 82 c1 81 49 81 76");
  auto parsed = parser.ParseBytecode(parsable);
  if (auto textoutElement = dynamic_cast<TextoutElement*>(parsed)) {
    std::wstring text = L"【声】「きょーすけが帰ってきたぞーっ！」";
    Cp932 encoding;
    EXPECT_EQ(encoding.ConvertString(textoutElement->GetText()), text);
  } else {
    ADD_FAILURE();
  }
  delete parsed;
}

TEST(TextoutParserTest, ParseQuotedEnglishString) {
  std::string s = "\"Say \\\"Hello.\\\"\"";
  std::ostringstream ss;
  for (const char& c : s) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << int(reinterpret_cast<const unsigned char&>(c));
    ss << ' ';
  }

  Parser parser;
  auto parsed = parser.ParseBytecode(PrintableToParsableString(ss.str()));
  EXPECT_EQ(16, parsed->GetBytecodeLength());
  if (auto textoutElement = dynamic_cast<TextoutElement*>(parsed)) {
    EXPECT_EQ(textoutElement->GetText(), "Say \"Hello.\""s);
  } else {
    ADD_FAILURE();
  }
  delete parsed;
}

// -----------------------------------------------------------------------
// MetaParserTest
// -----------------------------------------------------------------------

TEST(MetaParserTest, ParseLineElement) {
  Parser parser;
  {
    std::string parsable = PrintableToParsableString("0a 10 00");
    auto parsed = parser.ParseBytecode(parsable);
    if (auto lineElement = dynamic_cast<MetaElement*>(parsed)) {
      auto repr = lineElement->GetSourceRepresentation(nullptr);
      EXPECT_EQ(repr, "#line 16"s);
    } else {
      ADD_FAILURE()
          << "Parser failed to produce MetaElement object from '#line 16'";
    }
    delete parsed;
  }

  {
    std::string parsable = PrintableToParsableString("0a ff ff");
    auto parsed = parser.ParseBytecode(parsable);
    if (auto lineElement = dynamic_cast<MetaElement*>(parsed)) {
      auto repr = lineElement->GetSourceRepresentation(nullptr);
      EXPECT_EQ(repr, "#line 65535"s);
    } else {
      ADD_FAILURE()
          << "Parser failed to produce MetaElement object from '#line 65535'";
    }
    delete parsed;
  }
}

TEST(MetaParserTest, ParseEntrypointElement) {
  auto cdata = std::make_shared<ConstructionData>();
  cdata->kidoku_table.push_back(1000000 + 564);
  Parser parser(cdata);

  std::string parsable = PrintableToParsableString("21 00 00");
  auto parsed = parser.ParseBytecode(parsable);
  if (auto entrypointElement = dynamic_cast<MetaElement*>(parsed)) {
    auto repr = entrypointElement->GetSourceRepresentation(nullptr);
    EXPECT_EQ(repr, "#entrypoint 0"s);
    EXPECT_EQ(entrypointElement->GetEntrypoint(), 564);
  } else {
    ADD_FAILURE()
        << "Parser failed to produce MetaElement object from '#entrypoint 0'";
  }
  delete parsed;
}

TEST(MetaParserTest, ParseKidoku) {
  auto cdata = std::make_shared<ConstructionData>();
  cdata->kidoku_table.resize(4);
  cdata->kidoku_table[3] = 12;

  Parser parser(cdata);
  std::string parsable = PrintableToParsableString("40 03 00");
  auto parsed = parser.ParseBytecode(parsable);
  if (auto kidokuElement = dynamic_cast<MetaElement*>(parsed)) {
    auto repr = kidokuElement->GetSourceRepresentation(nullptr);
    EXPECT_EQ(repr, "{- Kidoku 3 -}");
  } else {
    ADD_FAILURE()
        << "Parser failed to produce MetaElement object from '{- Kidoku 3 -}'";
  }
  delete parsed;
}

// -----------------------------------------------------------------------
// ExpressionParserTest
// -----------------------------------------------------------------------

TEST(ExpressionParserTest, Assignment) {
  Parser parser;

  {
    std::string parsable = PrintableToParsableString(
        "$ 03 [ $ ff 63 02 00 00 ] 5c 1e $ ff 01 00 00 00");
    auto parsed =
        dynamic_cast<ExpressionElement*>(parser.ParseBytecode(parsable));
    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->GetSourceRepresentation(nullptr), "intD[611] = 1"s);
    delete parsed;
  }

  {
    std::string parsable = PrintableToParsableString(
        "$ 0b [ $ ff 00 00 00 00 ] 5c 14 $ ff 01 00 00 00");
    auto parsed =
        dynamic_cast<ExpressionElement*>(parser.ParseBytecode(parsable));
    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->GetSourceRepresentation(nullptr), "intL[0] += 1"s);
    delete parsed;
  }
}
