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

#include <gtest/gtest.h>

#include "base/compression.h"

#include <string>
#include <string_view>

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

TEST(lzssTest, NullOriginal) {
  {
    uint8_t compressed[] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::string_view compressed_data(reinterpret_cast<char*>(compressed),
                                     sizeof(compressed) / sizeof(uint8_t));
    std::string result = Decompress_lzss(compressed_data);

    EXPECT_EQ(result.size(), 0);
  }

  {
    std::string result = Decompress_lzss(""sv);
    EXPECT_EQ(result.size(), 0);
  }
}

TEST(lzssTest, literals) {
  uint8_t compressed[] = {
      0x0d, 0x00, 0x00, 0x00,  // archive size = 13
      0x04, 0x00, 0x00, 0x00,  // original size = 4
      0x0f,                    // flag: 0x0f
      0x41, 0x42, 0x43, 0x44   // literals: "ABCD"
  };

  std::string_view compressed_data(reinterpret_cast<char*>(compressed),
                                   sizeof(compressed) / sizeof(uint8_t));
  std::string result = Decompress_lzss(compressed_data);

  EXPECT_EQ(result, "ABCD"s);
}

TEST(lzssTest, backRef) {
  uint8_t compressed[] = {
      0x0e, 0x00, 0x00, 0x00,  // archive size = 14
      0x06, 0x00, 0x00, 0x00,  // original size = 6
      0x07,                    // flag: 0x07
      0x41, 0x42, 0x43,        // literals "ABC"
      0x31, 0x00               // back reference
  };

  std::string_view compressed_data(reinterpret_cast<char*>(compressed),
                                   sizeof(compressed) / sizeof(uint8_t));
  std::string result = Decompress_lzss(compressed_data);

  EXPECT_EQ(result, "ABCABC"s);
}

TEST(lzssTest, InvalidHeader) {
  uint8_t compressed[] = {0x00, 0x01};

  std::string_view compressed_data(reinterpret_cast<char*>(compressed),
                                   sizeof(compressed));
  EXPECT_THROW(Decompress_lzss(compressed_data), std::invalid_argument);
}

TEST(lzssTest, IncorrectArchiveSize) {
  uint8_t compressed[] = {0x0f, 0x00, 0x00, 0x00,  // archive size = 15
                          0x04, 0x00, 0x00, 0x00,  // original size = 4
                          0x0f, 0x41, 0x42, 0x43, 0x44};

  std::string_view compressed_data(reinterpret_cast<char*>(compressed), 13);
  EXPECT_THROW(Decompress_lzss(compressed_data), std::logic_error);
}

TEST(lzssTest, OverlappingBackRefs) {
  // Test with overlapping back references
  uint8_t compressed[] = {
      0x11,       0x00, 0x00, 0x00,  // archive size = 19
      0x0C,       0x00, 0x00, 0x00,  // original size = 10
      0b00001111,                    // flag
      0x41,       0x42, 0x43, 0x44,  // literals: 'A', 'B', 'C', 'D'
      0x40,       0x00,  // back reference to 'AB' (offset -4, length 2)
      0x44,       0x00   // back reference to 'CDABCD' (offset -4, length 6)
  };

  std::string_view compressed_data(reinterpret_cast<char*>(compressed),
                                   sizeof(compressed));
  std::string result = Decompress_lzss(compressed_data);

  EXPECT_EQ(result, "ABCDABCDABCD");
}
