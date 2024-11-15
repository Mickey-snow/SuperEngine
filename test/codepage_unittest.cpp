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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "encodings/codepage.hpp"
#include "encodings/cp932.hpp"
#include "encodings/cp936.hpp"
#include "encodings/cp949.hpp"
#include "encodings/western.hpp"

#include <string>

class Cp932Test : public ::testing::Test {
 protected:
  Cp932 cp932;
};

class Cp936Test : public ::testing::Test {
 protected:
  Cp936 cp936;
};

class Cp949Test : public ::testing::Test {
 protected:
  Cp949 cp949;
};

class Cp1252Test : public ::testing::Test {
 protected:
  Cp1252 cp1252;
};

TEST_F(Cp932Test, Convert) {
  unsigned short input = 0x3042;
  unsigned short output = cp932.Convert(input);
  EXPECT_EQ(output, 48);
}

TEST_F(Cp932Test, ConvertString) {
  std::string input = "\x82\xa0";  // Shift_JIS
  std::wstring expected = L"あ";
  std::wstring output = cp932.ConvertString(input);
  EXPECT_EQ(output, expected);
}

TEST_F(Cp936Test, JisDecode) {
  unsigned short input = 0xA1A1;  // GBK code
  unsigned short output = cp936.JisDecode(input);
  EXPECT_EQ(output, 53729);
}

TEST_F(Cp936Test, JisEncodeString) {
  const char* input = "测试";  // UTF-8
  char output[10];
  cp936.JisEncodeString(input, output, sizeof(output));
  const char* expected = "\x8B\xCB____";
  EXPECT_STREQ(output, expected);
}

TEST_F(Cp949Test, Convert) {
  unsigned short input = 0xAC00;  // Hangul syllable
  unsigned short output = cp949.Convert(input);
  EXPECT_EQ(output, 12478);
}

TEST_F(Cp949Test, ConvertString) {
  std::string input = "\xB0\xA1";  //  EUC-KR
  std::wstring expected = L"가";
  std::wstring output = cp949.ConvertString(input);
  EXPECT_EQ(output, expected);
}

TEST_F(Cp1252Test, DbcsDelim) {
  char input[] = "test";
  bool result = cp1252.DbcsDelim(input);
  EXPECT_FALSE(result) << "should be false since Cp1252 is SBCS";
}

TEST_F(Cp1252Test, IsItalic) {
  unsigned short input = 'A';
  bool result = cp1252.IsItalic(input);
  EXPECT_FALSE(result);
}
