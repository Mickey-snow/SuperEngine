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

#include "libsiglus/lexer.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace libsiglus {

enum class CommandCode : uint8_t {
  None = 0x00,

};

Element Lexer::Parse(std::string_view data) const {
  switch (static_cast<CommandCode>(data[0])) {
    case CommandCode::None:
      break;

    default: {
      std::stringstream ss;
      ss << "Lexer: Unable to parse " << '[';
      static constexpr size_t debug_length = 128;
      for (size_t i = 0; i < debug_length; ++i) {
        if (data.empty())
          break;
        ss << std::setfill('0') << std::setw(2) << std::hex
           << static_cast<int>(data[0]) << ' ';
        data = data.substr(1);
      }
      if (!data.empty())
        ss << "...";
      ss << ']';
      throw std::runtime_error(ss.str());
    }
  }
  return nullptr;
}

}  // namespace libsiglus
