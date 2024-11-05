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

#include <gtest/gtest.h>

#include "memory/memory.hpp"
#include "memory/location.hpp"
#include "libreallive/intmemref.h"

#include <sstream>
#include <string>

TEST(MemlocTest, rlIntMemref) {
  auto ToString = [](libreallive::IntMemRef ref) -> std::string {
    std::ostringstream os;
    os << ref;
    return os.str();
  };

  {
    int bytecode = 27;  // IntBb
    int location = 0;
    auto ref = libreallive::IntMemRef(bytecode, location);
    EXPECT_EQ(ref.bank(), libreallive::INTB_LOCATION);
    EXPECT_EQ(ref.location(), 0);
    EXPECT_EQ(ref.type(), 1);
    EXPECT_EQ(ToString(ref), "intBb[0]");

    bytecode = 11 + 2 * 26;  // IntL2b
    location = 7;
    ref = libreallive::IntMemRef(bytecode, location);
    EXPECT_EQ(ref.bank(), libreallive::INTL_LOCATION);
    EXPECT_EQ(ref.location(), 7);
    EXPECT_EQ(ref.type(), 2);
    EXPECT_EQ(ToString(ref), "intL2b[7]");
  }

  {
    int location = 512;
    auto ref = libreallive::IntMemRef('B', location);
    EXPECT_EQ(ref.bank(), libreallive::INTB_LOCATION);
    EXPECT_EQ(ref.location(), location);
    EXPECT_EQ(ref.type(), 0);
    EXPECT_EQ(ToString(ref), "intB[512]");
  }

  {
    int location = 623;
    auto ref = libreallive::IntMemRef('L', "4b", location);
    EXPECT_EQ(ref.bank(), libreallive::INTL_LOCATION);
    EXPECT_EQ(ref.location(), location);
    EXPECT_EQ(ref.type(), 3);
    EXPECT_EQ(ToString(ref), "intL4b[623]");
  }
}

TEST(MemlocTest, IntLocations) {
  auto a3 = IntMemoryLocation(IntBank::A, 3);
  EXPECT_EQ(static_cast<std::string>(a3), "intA[3]");
  auto x32_2b = IntMemoryLocation(IntBank::X, 32, 2);
  EXPECT_EQ(static_cast<std::string>(x32_2b), "intX2b[32]");
  auto l128_4b = IntMemoryLocation(libreallive::IntMemRef('L', "4b", 128));
  EXPECT_EQ(static_cast<std::string>(l128_4b), "intL4b[128]");
  auto e0_8b = IntMemoryLocation(libreallive::IntMemRef('E', "8b", 0));
  EXPECT_EQ(static_cast<std::string>(e0_8b), "intE8b[0]");
}

TEST(MemlocTest, StrLocations) {
  auto s2 = StrMemoryLocation(StrBank::S, 2);
  EXPECT_EQ(static_cast<std::string>(s2), "strS[2]");
  auto k0 = StrMemoryLocation(StrBank::K, 0);
  EXPECT_EQ(static_cast<std::string>(k0), "strK[0]");
}
