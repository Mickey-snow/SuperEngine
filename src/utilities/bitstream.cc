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

#include "bitstream.h"

#include <algorithm>
#include <stdexcept>

BitStream::BitStream(uint8_t* data, size_t length)
    : data_(data), length_(length * 8), number_(0), tail_pos_(0) {
  for (int i = 0; i < length && i < 8; ++i) {
    number_ |= static_cast<uint64_t>(data_[i]) << (i * 8);
    tail_pos_ += 8;
  }
}

uint64_t BitStream::Readbits(const int& bitwidth) {
  if (bitwidth < 0 || bitwidth > 64)
    throw std::invalid_argument("Bit width must be between 0 and 64");

  if (bitwidth == 64)
    return number_;
  uint64_t mask = (1ull << bitwidth) - 1;
  return number_ & mask;
}

uint64_t BitStream::Popbits(const int& bitwidth) {
  auto ret = Readbits(bitwidth);

  if (bitwidth == 64)
    number_ = 0;
  else
    number_ >>= bitwidth;

  if (tail_pos_ < length_) {
    uint64_t sigbit = (data_[tail_pos_ / 8] >> (tail_pos_ % 8));
    int shift = 8 - (int)(tail_pos_ % 8);
    for (; shift < bitwidth; shift += 8) {
      if (tail_pos_ + shift >= length_)
        break;

      sigbit |= data_[(tail_pos_ + shift) / 8] << shift;
    }
    if (bitwidth < 64)
      sigbit &= (1ull << bitwidth) - 1;
    number_ |= sigbit << (64 - bitwidth);
    tail_pos_ += bitwidth;
  }

  return ret;
}
