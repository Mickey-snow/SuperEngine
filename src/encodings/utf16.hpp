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

#ifndef SRC_ENCODINGS_UTF16_HPP
#define SRC_ENCODINGS_UTF16_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

inline std::u16string_view sv_to_u16sv(std::string_view sv) {
  return std::u16string_view(reinterpret_cast<const char16_t*>(sv.data()),
                             sv.size() / sizeof(char16_t));
}

class utf16le {
 public:
  // decode from raw bytes
  static std::string Decode(std::string_view);
  static std::string Decode(std::vector<uint8_t>);

  // decode from u16string
  static std::string Decode(const std::u16string&);
  static std::string Decode(std::u16string_view);
};

#endif
