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

#include "core/memory_internal/location.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

#include "libreallive/intmemref.hpp"

IntBank ToIntBank(char c) {
  switch (c) {
    case 'A':
      return IntBank::A;
    case 'B':
      return IntBank::B;
    case 'C':
      return IntBank::C;
    case 'D':
      return IntBank::D;
    case 'E':
      return IntBank::E;
    case 'F':
      return IntBank::F;
    case 'X':
      return IntBank::X;
    case 'G':
      return IntBank::G;
    case 'Z':
      return IntBank::Z;
    case 'H':
      return IntBank::H;
    case 'I':
      return IntBank::I;
    case 'J':
      return IntBank::J;
    case 'L':
      return IntBank::L;
    default:
      throw std::invalid_argument("Invalid character for IntBank: " +
                                  std::string(1, c));
  }
}

std::string ToString(IntBank bank, uint8_t bits) {
  std::string result = "int";

  switch (bank) {
    case IntBank::A:
      result += 'A';
      break;
    case IntBank::B:
      result += 'B';
      break;
    case IntBank::C:
      result += 'C';
      break;
    case IntBank::D:
      result += 'D';
      break;
    case IntBank::E:
      result += 'E';
      break;
    case IntBank::F:
      result += 'F';
      break;
    case IntBank::X:
      result += 'X';
      break;
    case IntBank::G:
      result += 'G';
      break;
    case IntBank::Z:
      result += 'Z';
      break;
    case IntBank::H:
      result += 'H';
      break;
    case IntBank::I:
      result += 'I';
      break;
    case IntBank::J:
      result += 'J';
      break;
    case IntBank::L:
      result += 'L';
      break;
    default:
      return "{Invalid int bank #" + std::to_string(static_cast<int>(bank)) +
             '}';
      break;
  }

  if (bits != 32 && bits != 0)
    result += std::to_string(static_cast<int>(bits)) + 'b';

  return result;
}

std::string ToString(StrBank bank) {
  std::string result;

  if (bank == StrBank::local_name)
    return "LocalName";
  else if (bank == StrBank::global_name)
    return "GlobalName";
  else {
    result += "str";
    switch (bank) {
      case StrBank::S:
        result += 'S';
        break;
      case StrBank::M:
        result += 'M';
        break;
      case StrBank::K:
        result += 'K';
        break;
      default:
        return "{Invalid str bank #" + std::to_string(static_cast<int>(bank)) +
               '}';
        break;
    }
  }

  return result;
}

// -----------------------------------------------------------------------
// class IntMemoryLocation
// -----------------------------------------------------------------------

IntMemoryLocation::IntMemoryLocation(IntBank bank,
                                     size_t location,
                                     uint8_t bits)
    : bank_(bank), location_(location), bits_(bits) {
  if (bits_ == 0)
    bits_ = 32;
}

IntMemoryLocation::IntMemoryLocation(libreallive::IntMemRef rlint) {
  auto bankid = rlint.bank();
  switch (bankid) {
    case libreallive::INTG_LOCATION:
      bank_ = IntBank::G;
      break;
    case libreallive::INTZ_LOCATION:
      bank_ = IntBank::Z;
      break;
    case libreallive::INTL_LOCATION:
      bank_ = IntBank::L;
      break;
    default:
      if (0 <= bankid && bankid <= 5)
        bank_ = static_cast<IntBank>(bankid);
      else
        throw std::invalid_argument(
            "IntMemoryLocation: invalid libreallive bank id " +
            std::to_string(bankid));
  }

  location_ = rlint.location();

  auto access_type = rlint.type();
  switch (access_type) {
    case 0:
      bits_ = 32;
      break;
    default:
      if (access_type < 0 || access_type > 4)
        throw std::invalid_argument(
            "IntMemoryLocation: invalid libreallive access type " +
            std::to_string(access_type));
      bits_ = 1 << (access_type - 1);
  }
}

IntBank IntMemoryLocation::Bank() const { return bank_; }
size_t IntMemoryLocation::Index() const { return location_; }
uint8_t IntMemoryLocation::Bitwidth() const { return bits_; }

std::ostream& operator<<(std::ostream& os, const IntMemoryLocation& memref) {
  os << ToString(memref.bank_, memref.Bitwidth());
  os << '[' << memref.location_ << ']';
  return os;
}

IntMemoryLocation::operator std::string() const {
  std::ostringstream oss;
  oss << *this;
  return oss.str();
}

bool IntMemoryLocation::operator<(const IntMemoryLocation& other) const {
  return std::make_tuple(bank_, location_, bits_) <
         std::make_tuple(other.bank_, other.location_, other.bits_);
}

bool IntMemoryLocation::operator==(const IntMemoryLocation& other) const {
  return bank_ == other.bank_ && location_ == other.location_ &&
         bits_ == other.bits_;
}

// -----------------------------------------------------------------------
// class StrMemoryLocation
// -----------------------------------------------------------------------

StrMemoryLocation::StrMemoryLocation(StrBank bank, size_t location)
    : bank_(bank), location_(location) {}

StrMemoryLocation::StrMemoryLocation(int bank, size_t location)
    : bank_(), location_(location) {
  switch (bank) {
    case libreallive::STRK_LOCATION:
      bank_ = StrBank::K;
      break;
    case libreallive::STRM_LOCATION:
      bank_ = StrBank::M;
      break;
    case libreallive::STRS_LOCATION:
      bank_ = StrBank::S;
      break;
    default:
      throw std::invalid_argument(
          "StrMemoryLocation: unknown libreallive string bank enum " +
          std::to_string(bank));
  }
}

StrBank StrMemoryLocation::Bank() const { return bank_; }
size_t StrMemoryLocation::Index() const { return location_; }

std::ostream& operator<<(std::ostream& os, const StrMemoryLocation& memref) {
  os << ToString(memref.Bank());
  os << '[' << memref.location_ << ']';

  return os;
}

StrMemoryLocation::operator std::string() const {
  std::ostringstream oss;
  oss << *this;
  return oss.str();
}

bool StrMemoryLocation::operator<(const StrMemoryLocation& other) const {
  return std::make_tuple(bank_, location_) <
         std::make_tuple(other.bank_, other.location_);
}

bool StrMemoryLocation::operator==(const StrMemoryLocation& other) const {
  return bank_ == other.bank_ && location_ == other.location_;
}
