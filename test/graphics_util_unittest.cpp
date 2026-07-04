// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include <utilities/graphics.hpp>

constexpr float EPS = 1e-6f;

TEST(RotateAroundTest, ZeroDegreeRotationLeavesPointUnchanged) {
  auto [x, y] = RotateAround(5, -3, 2, 7, 0);

  EXPECT_FLOAT_EQ(x, 5);
  EXPECT_FLOAT_EQ(y, -3);
}

TEST(RotateAroundTest, CenterPointIsInvariantUnderAnyRotation) {
  auto [x, y] = RotateAround(10, 20, 10, 20, 123);

  EXPECT_FLOAT_EQ(x, 10);
  EXPECT_FLOAT_EQ(y, 20);
}

TEST(RotateAroundTest, PositiveNinetyDegreesAroundOrigin) {
  auto [x, y] = RotateAround(1, 0, 0, 0, 90);

  EXPECT_NEAR(x, 0, EPS);
  EXPECT_NEAR(y, 1, EPS);
}

TEST(RotateAroundTest, PositiveNinetyDegreesAroundNonOriginCenter) {
  auto [x, y] = RotateAround(12, 10, 10, 10, 90);

  EXPECT_NEAR(x, 10, EPS);
  EXPECT_NEAR(y, 12, EPS);
}

TEST(RotateAroundTest, NegativeNinetyDegreesUsesOppositeDirection) {
  auto [x, y] = RotateAround(0, 1, 0, 0, -90);

  EXPECT_NEAR(x, 1, EPS);
  EXPECT_NEAR(y, 0, EPS);
}
