// -----------------------------------------------------------------------
//
// This file is part of RLVM
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

#include "libsiglus/gexedat.hpp"

#include "core/compression.hpp"
#include "core/gameexe.hpp"
#include "encodings/utf16.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <iostream>
#include <string_view>

namespace libsiglus {

Gameexe CreateGexe(std::string_view sv, const XorKey& key) {
  std::string data(sv.substr(8));
  [[maybe_unused]] int version = -1;
  int encryption = 0;

  {
    ByteReader reader(sv.substr(0, 8));
    version = reader.PopAs<int32_t>(4);
    encryption = reader.PopAs<int32_t>(4);
  }

  if (encryption) {
    for (size_t i = 0; i < data.size(); ++i)
      data[i] ^= key.exekey[i & 0xf];
  }

  static constexpr uint8_t gexe_key[256] = {
      0xD8, 0x29, 0xB9, 0x16, 0x3D, 0x1A, 0x76, 0xD0, 0x87, 0x9B, 0x2D, 0x0C,
      0x7B, 0xD1, 0xA9, 0x19, 0x22, 0x9F, 0x91, 0x73, 0x6A, 0x35, 0xB1, 0x7E,
      0xD1, 0xB5, 0xE7, 0xE6, 0xD5, 0xF5, 0x06, 0xD6, 0xBA, 0xBF, 0xF3, 0x45,
      0x3F, 0xF1, 0x61, 0xDD, 0x4C, 0x67, 0x6A, 0x6F, 0x74, 0xEC, 0x7A, 0x6F,
      0x26, 0x74, 0x0E, 0xDB, 0x27, 0x4C, 0xA5, 0xF1, 0x0E, 0x2D, 0x70, 0xC4,
      0x40, 0x5D, 0x4F, 0xDA, 0x9E, 0xC5, 0x49, 0x7B, 0xBD, 0xE8, 0xDF, 0xEE,
      0xCA, 0xF4, 0x92, 0xDE, 0xE4, 0x76, 0x10, 0xDD, 0x2A, 0x52, 0xDC, 0x73,
      0x4E, 0x54, 0x8C, 0x30, 0x3D, 0x9A, 0xB2, 0x9B, 0xB8, 0x93, 0x29, 0x55,
      0xFA, 0x7A, 0xC9, 0xDA, 0x10, 0x97, 0xE5, 0xB6, 0x23, 0x02, 0xDD, 0x38,
      0x4C, 0x9B, 0x1F, 0x9A, 0xD5, 0x49, 0xE9, 0x34, 0x0F, 0x28, 0x2D, 0x1B,
      0x52, 0x39, 0x5C, 0x36, 0x89, 0x56, 0xA7, 0x96, 0x14, 0xBE, 0x2E, 0xC5,
      0x3E, 0x08, 0x5F, 0x47, 0xA9, 0xDF, 0x88, 0x9F, 0xD4, 0xCC, 0x69, 0x1F,
      0x30, 0x9F, 0xE7, 0xCD, 0x80, 0x45, 0xF3, 0xE7, 0x2A, 0x1D, 0x16, 0xB2,
      0xF1, 0x54, 0xC8, 0x6C, 0x2B, 0x0D, 0xD4, 0x65, 0xF7, 0xE3, 0x36, 0xD4,
      0xA5, 0x3B, 0xD1, 0x79, 0x4C, 0x54, 0xF0, 0x2A, 0xB4, 0xB2, 0x56, 0x45,
      0x2E, 0xAB, 0x7B, 0x88, 0xC5, 0xFA, 0x74, 0xAD, 0x03, 0xB8, 0x9E, 0xD5,
      0xF5, 0x6F, 0xDC, 0xFA, 0x44, 0x49, 0x31, 0xF6, 0x83, 0x32, 0xFF, 0xC2,
      0xB1, 0xE9, 0xE1, 0x98, 0x3D, 0x6F, 0x31, 0x0D, 0xAC, 0xB1, 0x08, 0x83,
      0x9D, 0x0D, 0x10, 0xD1, 0x41, 0xF9, 0x00, 0xBA, 0x1A, 0xCF, 0x13, 0x71,
      0xE4, 0x86, 0x21, 0x2F, 0x23, 0x65, 0xC3, 0x45, 0xA0, 0xC3, 0x92, 0x48,
      0x9D, 0xEA, 0xDD, 0x31, 0x2C, 0xE9, 0xE2, 0x10, 0x22, 0xAA, 0xE1, 0xAD,
      0x2C, 0xC4, 0x2D, 0x7F};
  for (size_t i = 0; i < data.size(); ++i)
    data[i] ^= gexe_key[i & 0xff];

  data = Decompress_lzss(data);

  Gameexe gexe;
  std::stringstream ss(utf16le::Decode(data));
  std::string line;
  while (std::getline(ss, line, '\n')) {
    boost::trim(line);
    if (line.empty())
      continue;

    gexe.parseLine(line);
  }

  return gexe;
}

}  // namespace libsiglus