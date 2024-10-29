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

#include "libsiglus/element.hpp"
#include "libsiglus/types.hpp"
#include "utilities/byte_reader.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace libsiglus {

enum class CommandCode : uint8_t {
  None = 0x00,

  Newline = 0x01,
  Push = 0x02,
  Pop = 0x03,
  Property = 0x05,

  Marker = 0x08,

  Goto = 0x10,
  Goto_true = 0x11,
  Goto_false = 0x12,

  Assign = 0x20,
  Op2 = 0x22,

  Cmd = 0x30,
};

Element Lexer::Parse(std::string_view data) const {
  ByteReader reader(data);
  switch (static_cast<CommandCode>(reader.PopAs<uint8_t>(1))) {
    case CommandCode::Newline: {
      const int linenum = reader.PopAs<int>(4);
      return std::make_shared<Line>(linenum);
    }

    case CommandCode::Push: {
      auto type = static_cast<Type>(reader.PopAs<int32_t>(4));
      return std::make_shared<Push>(type, reader.PopAs<int32_t>(4));
    }

    case CommandCode::Pop: {
      auto type = static_cast<Type>(reader.PopAs<int32_t>(4));
      return std::make_shared<Pop>(type);
    }

    case CommandCode::Property:
      return std::make_shared<Property>();

    case CommandCode::Marker:
      return std::make_shared<Marker>();

    case CommandCode::Op2: {
      auto ltype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto rtype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto op = static_cast<OperatorCode>(reader.PopAs<uint8_t>(1));
      return std::make_shared<Operate2>(ltype, rtype, op);
    }

    case CommandCode::Cmd: {
      auto v1 = reader.PopAs<int32_t>(4);
      auto v2 = reader.PopAs<int32_t>(4);
      auto v3 = reader.PopAs<int32_t>(4);
      auto v4 = reader.PopAs<int32_t>(4);
      return std::make_shared<Command>(v1, v2, v3, v4);
    }

    case CommandCode::Goto:
      return std::make_shared<Goto>(Goto::Condition::Uncondition,
                                    reader.PopAs<int32_t>(4));
    case CommandCode::Goto_true:
      return std::make_shared<Goto>(Goto::Condition::True,
                                    reader.PopAs<int32_t>(4));
    case CommandCode::Goto_false:
      return std::make_shared<Goto>(Goto::Condition::False,
                                    reader.PopAs<int32_t>(4));

    case CommandCode::Assign: {
      auto ltype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto rtype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto v1 = reader.PopAs<int32_t>(4);
      return std::make_shared<Assign>(ltype, rtype, v1);
    }

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
}

}  // namespace libsiglus
