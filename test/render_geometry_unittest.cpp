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

#include <core/render_geometry.hpp>

TEST(RenderStateTest, IdentityParentLeavesUnchanged) {
  RenderState child, parent = RenderState::Id();
  child.pos_x = 11, child.pos_y = 13;
  child.scale_x = 2, child.scale_y = 3;
  child.rotation_degrees = 15;

  RenderState expected = child;
  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual);
}

TEST(RenderStateTest, ParentTranslationAddedAfterLocalTransform) {
  RenderState child = RenderState::Id(), parent = RenderState::Id();
  child.pos_x = 10, child.pos_y = 20;
  parent.pos_x = 100, parent.pos_y = 200;

  parent.center_x = 1, parent.center_y = 2, parent.center_rep_x = 3,
  parent.center_rep_y = 4;
  // doesn't matter

  RenderState expected = child;
  expected.pos_x += 100, expected.pos_y += 200;
  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual);
}

TEST(RenderStateTest, ParentScaleAppliedAroundParentRepCenter) {
  RenderState child = RenderState::Id(), parent = RenderState::Id();
  child.pos_x = 15, child.pos_y = 25;
  parent.center_rep_x = 10, parent.center_rep_y = 20;
  parent.scale_x = 2, parent.scale_y = 3;

  parent.center_x = 999, parent.center_y = 9999;
  // doesn't matter

  const float x = (15 - 10) * 2 + 10;  // = 20
  const float y = (25 - 20) * 3 + 20;  // = 35
  RenderState expected = child;
  expected.pos_x = x, expected.pos_y = y;
  expected.scale_x = 2, expected.scale_y = 3;
  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual)
      << expected.GetDebugString() << actual.GetDebugString();
}

TEST(RenderStateTest, ParentRotationAppliedAroundParentRepCenter) {
  RenderState child = RenderState::Id(), parent = RenderState::Id();
  child.pos_x = 20, child.pos_y = 10;
  parent.center_rep_x = 10, parent.center_rep_y = 10;
  parent.pos_x = 100, parent.pos_y = 0;
  parent.rotation_degrees = 90;

  parent.center_x = 999, parent.center_y = 9999;
  // doesn't matter

  RenderState expected = child;
  expected.pos_x = 110, expected.pos_y = 20;
  expected.rotation_degrees = 90;
  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual)
      << expected.GetDebugString() << actual.GetDebugString();
}

TEST(RenderStateTest, FoldOrderIsScaleThenRotateThenTranslate) {
  RenderState child = RenderState::Id(), parent = RenderState::Id();
  child.pos_x = 1, child.pos_y = 1;
  parent.scale_x = 2;
  // after scale: (2,1)
  parent.rotation_degrees = 90;
  // after rotate: (-1,2)
  parent.pos_x = 5, parent.pos_y = 7;
  // after translate: (4, 9)

  RenderState expected = child;
  expected.pos_x = 4, expected.pos_y = 9;
  expected.rotation_degrees = 90;
  expected.scale_x = 2;
  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual)
      << expected.GetDebugString() << actual.GetDebugString();
}

TEST(RenderStateTest, ParentScaleAndRotationAccumulate) {
  RenderState child = RenderState::Id(), parent = RenderState::Id();

  child.scale_x = 3, child.scale_y = 4;
  child.rotation_degrees = 15;
  // previous fold

  child.center_x = 1, child.center_y = 2;
  child.center_rep_x = 3, child.center_rep_y = 4;

  parent.scale_x = 2, parent.scale_y = 5;
  parent.rotation_degrees = 30;

  RenderState expected = child;
  expected.scale_x = 6, expected.scale_y = 20;
  expected.rotation_degrees = 45;
  // accumulated from parent's scale and rotate

  RenderState actual = RenderState::Fold(child, parent);
  EXPECT_EQ(expected, actual)
      << expected.GetDebugString() << actual.GetDebugString();
}
