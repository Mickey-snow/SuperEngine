// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#pragma once

#include <boost/locale.hpp>

#include <stdexcept>
#include <string>

enum class Encoding { Ascii, UTF16, CP932, CP936, CP949 };

std::string EncodingToString(Encoding enc) {
  switch (enc) {
    case Encoding::Ascii:
      return "ASCII";
    case Encoding::UTF16:
      return "UTF-16";
    case Encoding::CP932:
      return "CP932";  // Japanese code page
    case Encoding::CP936:
      return "CP936";  // Simplified Chinese code page
    case Encoding::CP949:
      return "CP949";  // Korean code page
    default:
      return "Unknown";
  }
}

std::string ConvertEncoding(const std::string& input,
                            std::string from,
                            std::string to) {
  try {
    return boost::locale::conv::between(input, to, from);
  } catch (const boost::locale::conv::conversion_error& e) {
    throw std::runtime_error(std::string("Conversion error: ") + e.what());
  }
}

std::string ConvertEncoding(const std::string& input,
                            Encoding from,
                            Encoding to) {
  return ConvertEncoding(input, EncodingToString(from), EncodingToString(to));
}
