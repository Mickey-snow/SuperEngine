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

#include "encodings/utf16.hpp"

#include <boost/locale.hpp>

std::string utf16le::Decode(std::string_view sv) {
  return Decode(sv_to_u16sv(sv));
}

std::string utf16le::Decode(std::vector<uint8_t> vec) {
  return Decode(
      std::string_view(reinterpret_cast<const char*>(vec.data()), vec.size()));
}

std::string utf16le::Decode(const std::u16string& str) {
  return boost::locale::conv::utf_to_utf<char>(str);
}

std::string utf16le::Decode(std::u16string_view sv) {
  return Decode(std::u16string(sv));
}
