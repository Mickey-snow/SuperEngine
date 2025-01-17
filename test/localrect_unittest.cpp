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

#include <gtest/gtest.h>

#include "core/localrect.hpp"

/**
 * @brief Test case: No intersection
 *
 * Source rect: (0,0) to (10,10)
 * LocalCoord bounds: offset=(20,20), size=10x10 => covers (20,20) to (30,30)
 * They do not overlap => Should return false
 */
TEST(LocalRectTest, NoIntersection) {
  LocalRect localSystem(Rect::REC(20, 20, 10, 10));

  auto src = Rect::GRP(0, 0, 10, 10);
  auto dst = Rect::GRP(100, 100, 110, 110);

  bool result = localSystem.intersectAndTransform(src, dst);
  EXPECT_FALSE(result);

  EXPECT_EQ(src, Rect::GRP(0, 0, 10, 10))
      << "values should remain unchanged if no intersection";
  EXPECT_EQ(dst, Rect::GRP(100, 100, 110, 110))
      << "values should remain unchanged if no intersection";
}

/**
 * @brief Test case: Full overlap
 *
 * Source rect: (0,0) to (10,10)
 * LocalCoord bounds: offset=(0,0), size=10x10 => exactly (0,0) to (10,10)
 * They match perfectly => Should return true, and the transformation
 * should yield the same source and destination rectangles.
 */
TEST(LocalRectTest, FullOverlap) {
  LocalRect localSystem(Rect::REC(0, 0, 10, 10));

  auto src = Rect::GRP(0, 0, 10, 10);
  auto dst = Rect::GRP(50, 60, 60, 70);

  bool result = localSystem.intersectAndTransform(src, dst);
  EXPECT_TRUE(result);

  // Since the source matches exactly, no modifications except local offset
  // which is zero in this case. (src stays same)
  EXPECT_EQ(src, Rect::GRP(0, 0, 10, 10));
  // Destination should remain the same because there's no partial intersection
  EXPECT_EQ(dst, Rect::GRP(50, 60, 60, 70));
}

/**
 * @brief Test case: Partial intersection
 *
 * Source rect: (5, 5) to (15,15)
 * LocalCoord bounds: offset=(0,0), size=10x10 => covers (0,0) to (10,10)
 * Overlap region is from (5,5) to (10,10).
 */
TEST(LocalRectTest, PartialIntersection) {
  LocalRect localSystem(Rect::REC(0, 0, 10, 10));

  // Source goes beyond the localSystem area on the bottom-right
  auto src = Rect::GRP(5, 5, 15, 15);
  auto dst = Rect::GRP(100, 100, 200, 200);

  bool result = localSystem.intersectAndTransform(src, dst);
  EXPECT_TRUE(result);

  // The intersection in source space: (5,5) to (10,10)
  // => localSystem offset is (0,0), so local space is the same for the source
  EXPECT_EQ(src, Rect::GRP(5, 5, 10, 10));

  // We clipped the source's 10x10 region down to 5x5 in width & height
  // => 50% in both width and height => we expect the destination to also shrink
  //    in the same ratio from (100,100)-(200,200). That's 100x100 => 50% =>
  //    50x50
  EXPECT_EQ(dst, Rect::REC(100, 100, 50, 50))
      << "Should shrink to half the width";
}

/**
 * @brief Test case: Intersection on the boundary
 *
 * Source rect: (10,10) to (20,20)
 * LocalCoord bounds: offset=(10,10), size=10x10 => covers (10,10) to (20,20)
 * The overlap is exact, but only at the boundary starting at (10,10)
 */
TEST(LocalRectTest, BoundaryIntersection) {
  LocalRect localSystem(Rect::REC(10, 10, 10, 10));

  auto src = Rect::GRP(10, 10, 20, 20);
  auto dst = Rect::GRP(0, 0, 100, 100);

  bool result = localSystem.intersectAndTransform(src, dst);
  EXPECT_TRUE(result);

  // Intersection is the full rectangle in this case
  // But since local offset is (10,10), the final source coords
  // in local space become (0,0) to (10,10)
  EXPECT_EQ(static_cast<std::string>(src), "Rect(0, 0, Size(10, 10))");

  // The destination rect remains the same size because the entire source
  // intersects (just shifted). The ratio is 1:1, so no scaling.
  // The only difference might be due to potential rounding, but here it's
  // exact.
  EXPECT_EQ(static_cast<std::string>(dst), "Rect(0, 0, Size(100, 100))");
}
