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

#include "libsiglus/lexeme.hpp"
#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"
#include "utilities/byte_reader.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace libsiglus {

using namespace libsiglus::lex;

std::vector<Type> ParseArglist(ByteReader& reader) {
  int cnt = reader.PopAs<int32_t>(4);
  std::vector<Type> arglist(cnt);
  while (--cnt >= 0)
    arglist[cnt] = static_cast<Type>(reader.PopAs<uint32_t>(4));
  return arglist;
}

Lexeme Lexer::Parse(std::string_view data) const {
  ByteReader reader(data);
  return Parse(reader);
}

Lexeme Lexer::Parse(ByteReader& reader) const {
  switch (static_cast<ByteCode>(reader.PopAs<uint8_t>(1))) {
    case ByteCode::None:
      return None{};

    case ByteCode::Newline: {
      const int linenum = reader.PopAs<int>(4);
      return Line(linenum);
    }

    case ByteCode::Push: {
      auto type = static_cast<Type>(reader.PopAs<int32_t>(4));
      return Push(type, reader.PopAs<int32_t>(4));
    }

    case ByteCode::Pop: {
      auto type = static_cast<Type>(reader.PopAs<int32_t>(4));
      return Pop(type);
    }

    case ByteCode::Property:
      return Property();

    case ByteCode::Marker:
      return Marker();

    case ByteCode::Copy:
      return Copy(static_cast<Type>(reader.PopAs<int32_t>(4)));

    case ByteCode::CopyElm:
      return CopyElm();

    case ByteCode::Op1: {
      auto type = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto op = static_cast<OperatorCode>(reader.PopAs<uint8_t>(1));
      return Operate1(type, op);
    }
    case ByteCode::Op2: {
      auto ltype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto rtype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto op = static_cast<OperatorCode>(reader.PopAs<uint8_t>(1));
      return Operate2(ltype, rtype, op);
    }

    case ByteCode::Cmd: {
      int arglist_id = reader.PopAs<int32_t>(4);
      std::vector<Type> stackarg = ParseArglist(reader);

      int named_arg_cnt = reader.PopAs<int32_t>(4);
      std::vector<int> argtags(named_arg_cnt);
      while (--named_arg_cnt >= 0)
        argtags[named_arg_cnt] = reader.PopAs<int32_t>(4);

      Type return_type = static_cast<Type>(reader.PopAs<uint32_t>(4));
      return Command(arglist_id, std::move(stackarg), std::move(argtags),
                     return_type);
    }

    case ByteCode::Goto:
      return Goto(Goto::Condition::Unconditional, reader.PopAs<int32_t>(4));
    case ByteCode::Goto_true:
      return Goto(Goto::Condition::True, reader.PopAs<int32_t>(4));
    case ByteCode::Goto_false:
      return Goto(Goto::Condition::False, reader.PopAs<int32_t>(4));

    case ByteCode::Gosub_int: {
      int label = reader.PopAs<int32_t>(4);
      auto arglist = ParseArglist(reader);
      return Gosub(Type::Int, label, std::move(arglist));
    }
    case ByteCode::Gosub_str: {
      int label = reader.PopAs<int32_t>(4);
      auto arglist = ParseArglist(reader);
      return Gosub(Type::String, label, std::move(arglist));
    }

    case ByteCode::Assign: {
      auto ltype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto rtype = static_cast<Type>(reader.PopAs<int32_t>(4));
      auto v1 = reader.PopAs<int32_t>(4);
      return Assign(ltype, rtype, v1);
    }

    case ByteCode::Namae:
      return Namae();

    case ByteCode::End:
      return EndOfScene();

    case ByteCode::Text:
      return Textout(reader.PopAs<int32_t>(4));

    case ByteCode::Return:
      return Return(ParseArglist(reader));

    default: {
      std::stringstream ss;
      ss << "Lexer: Unable to parse " << '[';
      static constexpr size_t debug_length = 128;
      for (size_t i = 0; i < debug_length; ++i) {
        if (reader.Position() >= reader.Size())
          break;
        ss << std::setfill('0') << std::setw(2) << std::hex
           << reader.PopAs<int>(1) << ' ';
      }
      if (reader.Position() < reader.Size())
        ss << "...";
      ss << ']';
      throw std::runtime_error(ss.str());
    }
  }
}

}  // namespace libsiglus
