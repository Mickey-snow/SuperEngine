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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "core/object.hpp"
#include "core/object_internal/object_mutator.hpp"

class GraphicsObjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& param = obj.Param();

    param.is_visible = true;
    param.position_x = 10;
    param.position_y = 20;
  }

  GraphicsObject obj;
};

TEST_F(GraphicsObjectTest, MoveSemantics) {
  GraphicsObject other = std::move(obj);

  EXPECT_TRUE(other.Param().is_visible);
  EXPECT_EQ(other.Param().position_x, 10);
  EXPECT_EQ(other.Param().position_y, 20);

  EXPECT_FALSE(obj.Param().is_visible);
}

TEST_F(GraphicsObjectTest, Clone) {
  GraphicsObject other = obj.Clone();

  other.Param().position_x = 100;
  EXPECT_TRUE(other.Param().is_visible);
  EXPECT_EQ(other.Param().position_x, 100);
  EXPECT_EQ(other.Param().position_y, 20);
  EXPECT_EQ(obj.Param().position_x, 10);
}

TEST_F(GraphicsObjectTest, EndObjectMutatorMatching) {
  obj.AddObjectMutator(ObjectMutator({}, -1, "fade"));
  EXPECT_TRUE(obj.IsMutatorRunningMatching(-1, "fade"));

  obj.EndObjectMutatorMatching(-1, "fade", 0);
  EXPECT_FALSE(obj.IsMutatorRunningMatching(-1, "fade"));
}
