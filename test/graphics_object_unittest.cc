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

#include "systems/base/graphics_object.h"

class GraphicsObjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& param = obj.Param();

    param.Set(ObjectProperty::IsVisible, true);
    param.Set(ObjectProperty::PositionX, 10);
    param.Set(ObjectProperty::PositionY, 20);
  }

  GraphicsObject obj;
};

TEST_F(GraphicsObjectTest, MoveSemantics) {
  GraphicsObject other = std::move(obj);

  EXPECT_TRUE(other.Param().Get<ObjectProperty::IsVisible>());
  EXPECT_EQ(other.Param().Get<ObjectProperty::PositionX>(), 10);
  EXPECT_EQ(other.Param().Get<ObjectProperty::PositionY>(), 20);

  EXPECT_FALSE(obj.Param().Get<ObjectProperty::IsVisible>());
}

TEST_F(GraphicsObjectTest, Clone) {
  GraphicsObject other = obj.Clone();

  other.Param().Set(ObjectProperty::PositionX, 100);
  EXPECT_TRUE(other.Param().Get<ObjectProperty::IsVisible>());
  EXPECT_EQ(other.Param().Get<ObjectProperty::PositionX>(), 100);
  EXPECT_EQ(other.Param().Get<ObjectProperty::PositionY>(), 20);
  EXPECT_EQ(obj.Param().Get<ObjectProperty::PositionX>(), 10);
}
