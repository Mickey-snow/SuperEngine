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

#include "utilities/bitstream.h"

TEST(BitStreamTest, ReadBits) {
  char rawbits[] = {0b11111011, 0b01010000};
  const size_t N = sizeof(rawbits) / sizeof(char);

  BitStream bs((uint8_t*)rawbits, N);
  EXPECT_EQ(bs.Readbits(2), 0b11);
  EXPECT_EQ(bs.Readbits(3), 0b011);
  EXPECT_EQ(bs.Readbits(4), 0b1011);
  EXPECT_EQ(bs.Readbits(10), 0b11111011);
  EXPECT_EQ(bs.Readbits(13), 0b1000011111011);
}

TEST(BitStreamTest, PopBits) {
  unsigned int rawbits[] = {0b00000000000010001111100100000101,
                            0b10110000111110101010100100111011,
                            0b0010101000111111};
  const size_t N = sizeof(rawbits) / sizeof(int);

  BitStream bs(rawbits, rawbits + N);

  EXPECT_EQ(bs.Popbits(4), 0b0101);
  EXPECT_EQ(bs.Popbits(32), 0b10110000000000001000111110010000);
  EXPECT_EQ(bs.Popbits(11), 0b1010010011);
  EXPECT_EQ(bs.Popbits(50), 0b1010100011111110110000111110101);
}

TEST(BitStreamTest, EdgeWidth) {
  char rawbits[] = {0xab, 0x2d, 0x12, 0x33, 0x9a, 0xff,
                    0xf1, 0x2b, 0x7f, 0x46, 0xa9, 0x8c};
  const size_t N = sizeof(rawbits) / sizeof(char);

  BitStream bs(rawbits, rawbits+N);
  EXPECT_EQ(bs.Popbits(0), 0);
  EXPECT_EQ(bs.Popbits(3), 3);
  EXPECT_EQ(bs.Popbits(64), 16536725195841488309ull);
  EXPECT_EQ(bs.Popbits(64), 294987983ull);
}

TEST(BitStreamTest, InvalidBitwidth) {
  char rawbits[] = {0xab, 0x2d, 0x12, 0x33, 0x9a};
  const size_t N = sizeof(rawbits) / sizeof(char);

  BitStream bs(rawbits, rawbits + N);
  EXPECT_THROW(bs.Readbits(-1), std::invalid_argument);
  EXPECT_THROW(bs.Readbits(65), std::invalid_argument);
}
