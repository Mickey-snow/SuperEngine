// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include <iosfwd>

namespace libreallive {

[[maybe_unused]] const int INTA_LOCATION = 0;
[[maybe_unused]] const int INTB_LOCATION = 1;
[[maybe_unused]] const int INTC_LOCATION = 2;
[[maybe_unused]] const int INTD_LOCATION = 3;
[[maybe_unused]] const int INTE_LOCATION = 4;
[[maybe_unused]] const int INTF_LOCATION = 5;
[[maybe_unused]] const int INTG_LOCATION = 6;

[[maybe_unused]] const int INTZ_LOCATION = 7;
[[maybe_unused]] const int INTL_LOCATION = 8;

// -----------------------------------------------------------------------

[[maybe_unused]] const int INTZ_LOCATION_IN_BYTECODE = 25;
[[maybe_unused]] const int INTL_LOCATION_IN_BYTECODE = 11;

// -----------------------------------------------------------------------

const int STRK_LOCATION = 0x0A;
const int STRM_LOCATION = 0x0C;
const int STRS_LOCATION = 0x12;

// -----------------------------------------------------------------------

inline bool is_string_location(const int type) {
  return type == STRS_LOCATION || type == STRK_LOCATION ||
         type == STRM_LOCATION;
}

// -----------------------------------------------------------------------

// References a piece of integer memory.
class IntMemRef {
 public:
  IntMemRef(int bytecode_rep, int location);
  IntMemRef(int bank, int type, int location);

  IntMemRef(char bank_name, int location);
  IntMemRef(char bank_name, const char* access_str, int location);
  ~IntMemRef();

  int bank() const { return memory_bank_; }
  int type() const { return access_type_; }
  int location() const { return location_; }

 private:
  // Which piece of memory to operate on.
  int memory_bank_;

  // How to access
  int access_type_;

  // The memory location to
  int location_;
};

}  // namespace libreallive

std::ostream& operator<<(std::ostream& oss,
                         const libreallive::IntMemRef& memref);
