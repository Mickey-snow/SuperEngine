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

#include "memory/location.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

#include "libreallive/intmemref.h"

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
  os << "int";

  switch (memref.bank_) {
    case IntBank::A:
      os << 'A';
      break;
    case IntBank::B:
      os << 'B';
      break;
    case IntBank::C:
      os << 'C';
      break;
    case IntBank::D:
      os << 'D';
      break;
    case IntBank::E:
      os << 'E';
      break;
    case IntBank::F:
      os << 'F';
      break;
    case IntBank::X:
      os << 'X';
      break;
    case IntBank::G:
      os << 'G';
      break;
    case IntBank::Z:
      os << 'Z';
      break;
    case IntBank::H:
      os << 'H';
      break;
    case IntBank::I:
      os << 'I';
      break;
    case IntBank::J:
      os << 'J';
      break;
    case IntBank::L:
      os << 'L';
      break;
    case IntBank::CNT:
      os << "{Invalid bank CNT}";
      break;
    default:
      os << "{Invalid bank #" << static_cast<int>(memref.bank_) << '}';
      break;
  }

  if (memref.bits_ != 32) {
    os << static_cast<int>(memref.bits_) << 'b';
  }

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

StrBank StrMemoryLocation::Bank() const { return bank_; }
size_t StrMemoryLocation::Index() const { return location_; }

std::ostream& operator<<(std::ostream& os, const StrMemoryLocation& memref) {
  if (memref.bank_ == StrBank::local_name)
    os << "LocalName";
  else if (memref.bank_ == StrBank::global_name)
    os << "GlobalName";
  else {
    os << "str";
    switch (memref.bank_) {
      case StrBank::S:
        os << 'S';
        break;
      case StrBank::M:
        os << 'M';
        break;
      case StrBank::K:
        os << 'K';
        break;
      case StrBank::CNT:
        os << "{Invalid bank CNT}";
        break;
      default:
        os << "{Invalid bank #" << static_cast<int>(memref.bank_) << '}';
        break;
    }
  }

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
