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

#include <gtest/gtest.h>

#include "libsiglus/value.hpp"

namespace siglus_test {
using namespace libsiglus;

TEST(sgValueTest, Int) {
  Value it = Integer(12);
  EXPECT_EQ(ToString(it), "int:12");
  EXPECT_EQ(Typeof(it), Type::Int);
}

TEST(sgValueTest, String) {
  Value it = String("Hello, World!");
  EXPECT_EQ(ToString(it), "str:Hello, World!");
  EXPECT_EQ(Typeof(it), Type::String);
}

TEST(sgValueTest, Variable) {
  Value it = Variable(Type::Int, 123);
  EXPECT_EQ(ToString(it), "v123");
  EXPECT_EQ(Typeof(it), Type::Int);
}

}  // namespace siglus_test
