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

#include "libsiglus/elm_details/memory.hpp"

#include "libsiglus/element.hpp"

#include <memory>

namespace libsiglus::elm {

Element Memory::Parse(const ElementCode& elmcode) {
  auto result = std::make_unique<Memory>();
  result->bank = static_cast<Bank>(elmcode.At<int>(0));
  switch (result->bank) {
    case Bank::A:
    case Bank::B:
    case Bank::C:
    case Bank::D:
    case Bank::E:
    case Bank::F:
    case Bank::X:
    case Bank::G:
    case Bank::Z:
    case Bank::H:
    case Bank::I:
    case Bank::J:
    case Bank::L:
      result->type = Type::Int;
      break;
    default:
      result->type = Type::String;
      break;
  }

  auto path = elmcode.IntegerView();
  for (size_t i = 1; i < path.size();) {
    int it = path[i++];

    switch (it) {
      case -1: {
        result->var = Access(elmcode.code[i]);
        return result;
      }

      case 3:
        result->bits = 1;
        break;
      case 4:
        result->bits = 2;
        break;
      case 5:
        result->bits = 4;
        break;
      case 7:
        result->bits = 8;
        break;
      case 6:
        result->bits = 16;
        break;

      case 10:
        result->var = Init();
        return result;

      case 2:
        result->var = Resize();
        return result;
      case 9:
        result->var = Size();
        return result;

      case 8:
        result->var = Fill();
        return result;

      case 1:
        result->var = Set();
        return result;
    }
  }

  return result;
}

Kind Memory::Kind() const noexcept { return Kind::Memory; }
std::string Memory::ToDebugString() const {
  std::string repr;

  switch (bank) {
    case Bank::A:
      repr = "A";
      break;
    case Bank::B:
      repr = "B";
      break;
    case Bank::C:
      repr = "C";
      break;
    case Bank::D:
      repr = "D";
      break;
    case Bank::E:
      repr = "E";
      break;
    case Bank::F:
      repr = "F";
      break;
    case Bank::X:
      repr = "X";
      break;
    case Bank::G:
      repr = "G";
      break;
    case Bank::Z:
      repr = "Z";
      break;
    case Bank::S:
      repr = "S";
      break;
    case Bank::M:
      repr = "M";
      break;
    case Bank::H:
      repr = "H";
      break;
    case Bank::I:
      repr = "I";
      break;
    case Bank::J:
      repr = "J";
      break;
    case Bank::L:
      repr = "L";
      break;
    case Bank::K:
      repr = "K";
      break;
    case Bank::local_name:
      repr = "local_name";
      break;
    case Bank::global_name:
      repr = "global_name";
      break;
  }

  if (bits != 32)
    repr += std::to_string(bits);

  repr += std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::same_as<T, Access>)
          return '[' + ToString(v.idx) + ']';
        else if constexpr (std::same_as<T, Init>)
          return ".init";
        else if constexpr (std::same_as<T, Resize>)
          return ".resize";
        else if constexpr (std::same_as<T, Fill>)
          return ".fill";
        else if constexpr (std::same_as<T, Size>)
          return ".size";
        else if constexpr (std::same_as<T, Set>)
          return ".set";

        return "???";
      },
      var);

  return repr;
}

}  // namespace libsiglus::elm
