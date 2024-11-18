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

#include "encodings/cp932.hpp"
#include "libreallive/parser.hpp"

#include <iomanip>
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

// -----------------------------------------------------------------------
// CommaParserTest
// -----------------------------------------------------------------------

TEST(CommaParserTest, ParseCommaElement) {
  Parser parser;
  std::string parsable = PrintableToParsableString("00");
  auto parsed = parser.ParseBytecode(parsable);
  CommaElement const* commaElement = nullptr;
  EXPECT_NO_THROW({
    commaElement = std::get<CommaElement const*>(parsed->DownCast());
  }) << "Parser failed to produce CommaElement object from '<CommaElement>'";

  auto repr = commaElement->GetSourceRepresentation(nullptr);
  EXPECT_EQ(repr, "<CommaElement>"s);
  std::ostringstream oss;
  parsed->PrintSourceRepresentation(nullptr, oss);
  EXPECT_EQ(oss.str(), "<CommaElement>\n"s);

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
  TextoutElement const* textoutElement = nullptr;
  EXPECT_NO_THROW({
    textoutElement = std::get<TextoutElement const*>(parsed->DownCast());
  });

  std::wstring text = L"【声】「きょーすけが帰ってきたぞーっ！」";
  Cp932 encoding;
  EXPECT_EQ(encoding.ConvertString(textoutElement->GetText()), text);

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
  TextoutElement const* textoutElement = nullptr;
  EXPECT_NO_THROW({
    textoutElement = std::get<TextoutElement const*>(parsed->DownCast());
  });
  EXPECT_EQ(textoutElement->GetText(), "Say \"Hello.\""s);

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
    MetaElement const* lineElement = nullptr;
    EXPECT_NO_THROW(
        { lineElement = std::get<MetaElement const*>(parsed->DownCast()); });

    auto repr = lineElement->GetSourceRepresentation(nullptr);
    EXPECT_EQ(repr, "#line 16"s);

    delete parsed;
  }

  {
    std::string parsable = PrintableToParsableString("0a ff ff");
    auto parsed = parser.ParseBytecode(parsable);
    MetaElement const* lineElement = nullptr;
    EXPECT_NO_THROW(
        { lineElement = std::get<MetaElement const*>(parsed->DownCast()); });

    auto repr = lineElement->GetSourceRepresentation(nullptr);
    EXPECT_EQ(repr, "#line 65535"s);

    delete parsed;
  }
}

TEST(MetaParserTest, ParseEntrypointElement) {
  auto cdata = std::make_shared<BytecodeTable>();
  cdata->kidoku_table.push_back(1000000 + 564);
  Parser parser(cdata);

  std::string parsable = PrintableToParsableString("21 00 00");
  auto parsed = parser.ParseBytecode(parsable);
  MetaElement const* entrypointElement = nullptr;
  EXPECT_NO_THROW({
    entrypointElement = std::get<MetaElement const*>(parsed->DownCast());
  });

  auto repr = entrypointElement->GetSourceRepresentation(nullptr);
  EXPECT_EQ(repr, "#entrypoint 0"s);
  EXPECT_EQ(entrypointElement->GetEntrypoint(), 564);

  delete parsed;
}

TEST(MetaParserTest, ParseKidoku) {
  auto cdata = std::make_shared<BytecodeTable>();
  cdata->kidoku_table.resize(4);
  cdata->kidoku_table[3] = 12;

  Parser parser(cdata);
  std::string parsable = PrintableToParsableString("40 03 00");
  auto parsed = parser.ParseBytecode(parsable);
  MetaElement const* kidokuElement = nullptr;
  EXPECT_NO_THROW(
      { kidokuElement = std::get<MetaElement const*>(parsed->DownCast()); });
  auto repr = kidokuElement->GetSourceRepresentation(nullptr);
  EXPECT_EQ(repr, "{- Kidoku 3 -}");

  delete parsed;
}

// -----------------------------------------------------------------------
// ExpressionParserTest
// -----------------------------------------------------------------------

TEST(ExpressionParserTest, ExprToken) {
  ExpressionParser parser;

  // IntConstant
  {
    std::string parsable = PrintableToParsableString("ff 01 00 00 00");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetExpressionToken(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "1"s);
  }

  // StoreReg
  {
    std::string parsable = PrintableToParsableString("c8");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetExpressionToken(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "<store>"s);
  }

  // MemoryRef
  {
    std::string parsable = PrintableToParsableString("0b [ $ ff 02 00 00 00 ]");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetExpressionToken(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "intL[2]"s);
  }
}

TEST(ExpressionParserTest, Expr) {
  ExpressionParser parser;

  {
    std::string parsable = PrintableToParsableString(
        "$ 03 [ $ ff 54 01 00 00 5c 00 $ 03 [ $ ff fb 00 00 00 ] ] 5c 28 $ ff "
        "01 00 00 00");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetExpression(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "intD[340 + intD[251]] == 1"s);
  }

  {
    std::string parsable = PrintableToParsableString(
        "$ 0b [ $ ff 00 00 00 00 ] 5c 2b $ 03 [ $ ff 56 01 00 00 5c 00 $ 03 [ "
        "$ ff fa 00 00 00 ] ]");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetExpression(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "intL[0] < intD[342 + intD[250]]"s);
  }
}

TEST(ExpressionParserTest, Assignment) {
  ExpressionParser parser;

  {
    std::string parsable = PrintableToParsableString(
        "$ 03 [ $ ff 56 01 00 00 5c 00 $ 03 [ $ ff fa 00 00 00 ] ] 5c 15 $ 0b "
        "[ $ ff 02 00 00 00 ]");
    const char* src = parsable.c_str();
    Expression parsed = parser.GetAssignment(src);
    EXPECT_EQ(src, parsable.c_str() + parsable.size());
    EXPECT_EQ(parsed->GetDebugString(), "intD[342 + intD[250]] -= intL[2]"s);
  }
}

TEST(ExpressionParserTest, Data) {
  ExpressionParser parser;

  {
    std::string parsable = PrintableToParsableString(
        "( $ ff 00 05 00 00 5c 01 $ ff d0 02 00 00 ) 5c 03 $ ff 02 00 00 00");
    const char* start = parsable.c_str();
    const char* end = start;
    Expression parsed = parser.GetData(end);

    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(end - start, parsable.length());
    EXPECT_EQ(parsed->GetDebugString(), "280"s);
  }

  {
    // In later games, we found newline metadata inside special parameters(?)
    // Make sure that the expression parser can deal with that.
    std::string parsable = PrintableToParsableString(
        "0a 77 02 61 37 61 10 ( $ ff 29 00 00 00 5c 02 $ ff 8d 01 00 00 "
        "$ ff ff 00 00 00 )");

    Expression parsed;
    const char* pos = parsable.c_str();
    EXPECT_NO_THROW({ parsed = parser.GetData(pos); });

    ASSERT_TRUE(parsed->IsSpecialParameter());
    EXPECT_EQ(pos - parsable.c_str(), parsable.length());
    EXPECT_EQ(parsed->GetOverloadTag(), 1048631)
        << "Tag 'a 0x37 a 0x10' should have value ((0x10<<16) | 0x37)";
    EXPECT_EQ(parsed->GetDebugString(), "1048631:{16277, 255}"s);
  }
}

TEST(ExpressionParserTest, ComplexParam) {
  ExpressionParser parser;

  std::string parsable = PrintableToParsableString(
      "( $ ff 70 21 00 00 0a 00 00 $ ff 1e 00 00 00 61 00 $ 0b [ $ ff 0b 00 00 "
      "00 ] )");
  const char* src = parsable.c_str();
  auto parsed = parser.GetComplexParam(src);

  ASSERT_TRUE(parsed->IsComplexParameter());
  EXPECT_EQ(parsed->GetDebugString(), "(8560, 30, 0:{intL[11]})"s);

  auto exprs = parsed->GetContainedPieces();
  EXPECT_EQ(exprs.size(), 3);
}

// -----------------------------------------------------------------------
// CommandParserTest
// -----------------------------------------------------------------------

class CommandParserTest : public ::testing::Test {
 protected:
  void TearDown() override {
    for (auto& it : parsed_cmds)
      delete it;
  }

  using data_t = std::vector<std::pair<std::string, std::string>>;
  void TestWith(const data_t& data) {
    parsed_cmds.reserve(data.size());
    for (const auto& [printable, repr] : data) {
      const auto parsable = PrintableToParsableString(printable);
      CommandElement* parsed = parser.ParseCommand(parsable.c_str());
      ASSERT_NE(parsed, nullptr);
      EXPECT_EQ(parsed->GetBytecodeLength(), parsable.length());
      EXPECT_EQ(parsed->GetSourceRepresentation(nullptr), repr);
      parsed_cmds.push_back(parsed);
    }
  }

  Parser parser;
  std::vector<CommandElement*> parsed_cmds;
};

TEST_F(CommandParserTest, GotoElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 00 00 00 00 00 25 01 00 00"s, "op<0:001:00000, 0>() @293"s},
      {"23 00 01 05 00 00 00 00 a7 01 00 00"s, "op<0:001:00005, 0>() @423"s}};
  TestWith(data);
  auto cmd = parsed_cmds.front();
  EXPECT_EQ(cmd->GetParamCount(), 0);
}

TEST_F(CommandParserTest, GotoIfElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 02 00 00 00 00 ( $ 06 [ $ ff eb 03 00 00 ] 5c 28 $ ff 01 00 00 00 ) f3 00 00 00"s,
       "op<0:001:00002, 0>(intG[1003] == 1) @243"s}};
  TestWith(data);
  auto cmd = parsed_cmds.front();
  EXPECT_EQ(cmd->GetParamCount(), 1);
}

TEST_F(CommandParserTest, GotoOnElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 03 00 0e 00 00 ( $ 0b [ $ ff 00 00 00 00 ] ) { 44 02 00 00 91 02 00 00 de 02 00 00 2b 03 00 00 78 03 00 00 c5 03 00 00 12 04 00 00 5f 04 00 00 ac 04 00 00 f9 04 00 00 46 05 00 00 93 05 00 00 e0 05 00 00 2d 06 00 00 }"s,
       "op<0:001:00003, 0>(intL[0]){ @580 @657 @734 @811 @888 @965 @1042 @1119 @1196 @1273 @1350 @1427 @1504 @1581}"s},
      {"23 00 01 08 00 0a 00 00 ( $ 0b [ $ ff 01 00 00 00 ] ) { e7 60 00 00 a5 66 00 00 95 6a 00 00 99 6e 00 00 89 73 00 00 a3 77 00 00 a3 7b 00 00 9d 84 00 00 f6 88 00 00 2f 8d 00 00 }"s,
       "op<0:001:00008, 0>(intL[1]){ @24807 @26277 @27285 @28313 @29577 @30627 @31651 @33949 @35062 @36143}"s}};
  TestWith(data);
  {
    auto cmd = dynamic_cast<GotoOnElement*>(parsed_cmds[0]);
    ASSERT_NE(cmd, nullptr);
  }
  {
    auto cmd = dynamic_cast<GotoOnElement*>(parsed_cmds[1]);
    ASSERT_NE(cmd, nullptr);
  }
}

TEST_F(CommandParserTest, GotoCaseElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 04 00 03 00 00 ( $ 0b [ $ ff 00 00 00 00 ] ) { ( $ ff 00 00 00 00 ) 6d 08 00 00 ( $ ff 01 00 00 00 ) a1 08 00 00 ( ) d5 08 00 00 }"s,
       "op<0:001:00004, 0>(intL[0]) [0]@2157 [1]@2209 []@2261"s}};
  TestWith(data);
  auto cmd = parsed_cmds.front();
  EXPECT_EQ(cmd->GetCaseCount(), 3);
}

TEST_F(CommandParserTest, GosubWithElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 0a 00 00 00 00"s, "op<0:001:00010, 0>()"s},
      {"23 00 01 10 00 02 00 00 ( 61 00 $ 01 [ $ ff 00 00 00 00 ] 61 00 $ 01 [ $ ff 01 00 00 00 ] ) 56 01 00 00"s,
       "op<0:001:00016, 0>(0:{intB[0]}, 0:{intB[1]}) @342"s}};
  TestWith(data);
  EXPECT_EQ(parsed_cmds[0]->GetParamCount(), 0);
  EXPECT_EQ(parsed_cmds[1]->GetParamCount(), 2);
}

TEST_F(CommandParserTest, SelectElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 02 03 00 04 00 00 { 0a 4b 00 ( ( $ 0b [ $ ff 01 00 00 00 ] 5c ( 5c 01 $ ff 01 00 00 00 ) 32 ( $ 0b [ $ ff 01 00 00 00 ] 5c ( $ ff 8d 00 00 00 ) 31 $ ff 64 00 00 00 ) 23 23 23 50 52 49 4e 54 ( $ 12 [ $ ff 00 00 00 00 ] ) 0a 4c 00 ( ( $ 0b [ $ ff 02 00 00 00 ] 5c ( 5c 01 $ ff 01 00 00 00 ) 32 ( $ 0b [ $ ff 02 00 00 00 ] 5c ( $ ff 8d 00 00 00 ) 31 $ ff 64 00 00 00 ) 23 23 23 50 52 49 4e 54 ( $ 12 [ $ ff 02 00 00 00 ] ) 0a 4d 00 ( ( $ 0b [ $ ff 03 00 00 00 ] 5c ( 5c 01 $ ff 01 00 00 00 ) 32 ( $ 0b [ $ ff 03 00 00 00 ] 5c ( $ ff 8d 00 00 00 ) 31 $ ff 64 00 00 00 ) 23 23 23 50 52 49 4e 54 ( $ 12 [ $ ff 04 00 00 00 ] ) 0a 4e 00 ( ( $ 0b [ $ ff 0b 00 00 00 ] 5c ( 5c 01 $ ff 01 00 00 00 ) 32 ( $ 0b [ $ ff 0b 00 00 00 ] 5c ( $ ff 8d 00 00 00 ) 31 $ ff 64 00 00 00 ) 23 23 23 50 52 49 4e 54 ( $ 12 [ $ ff 06 00 00 00 ] ) 0a 4f 00 7d"s,
       "op<0:002:00003, 0>()"s}};
  TestWith(data);

  {
    auto sel = dynamic_cast<SelectElement*>(parsed_cmds.front());
    ASSERT_NE(sel, nullptr);
    EXPECT_EQ(sel->GetParamCount(), 4);
    auto param = sel->raw_params();
    ASSERT_EQ(param.size(), 4);
  }
}

TEST_F(CommandParserTest, FunctionElement) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 01 51 e8 03 03 00 00 "
       "28 24 ff 00 00 00 00 28 24 ff 00 05 00 00 5c 01 24 ff d0 02 00 00 29 "
       "5c 03 24 ff 02 00 00 00 28 24 ff c0 03 00 00 5c 01 24 ff f0 00 00 00 "
       "29 5c 03 24 ff 02 00 00 00 29"s,
       "op<1:081:01000, 0>(0, 280, 360)"s},
      {"23 01 04 6c 02 01 00 00 ( ( $ ff 00 00 00 00 $ ff 00 00 00 00 $ ff 10 27 00 00 $ 02 [ $ ff 00 00 00 00 ] ) )"s,
       "op<1:004:00620, 0>((0, 0, 10000, intC[0]))"s}};
  TestWith(data);
  // TODO: test for GetSerialized
}

TEST_F(CommandParserTest, FunctionElementWithMeta) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 01 15 28 00 08 00 00 ( $ ff 00 00 00 00 0a 04 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 05 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 06 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 07 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 08 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 09 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 0a 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 64 00 00 00 $ ff 64 00 00 00 ) 0a 0b 01 ( 42 54 5f 53 45 5f 41 30 30 41 $ ff 10 27 00 00 $ ff 10 27 00 00 ) 0a 0c 01 )"s,
       "op<1:021:00040, 0>(0, (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 100, 100), (\"BT_SE_A00A\", 10000, 10000))"s}};
  TestWith(data);
}

TEST_F(CommandParserTest, FunctionElementWithTag) {
  std::vector<std::pair<std::string, std::string>> data = {
      {"23 00 01 12 00 02 00 00 ( $ ff f2 1e 00 00 $ ff 0b 00 00 00 61 01 50 54 5f 41 4e 4e 30 31 61 01 50 54 5f 41 4e 4e 30 32 )"s,
       "op<0:001:00018, 0>(7922, 11, 1:{\"PT_ANN01\"}, 1:{\"PT_ANN02\"})"s}};
  TestWith(data);
}
