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

#include "core/object_internal/objdrawer.hpp"
#include "core/stage.hpp"

#include <limits>
#include <memory>

class StageTest : public ::testing::Test {
 protected:
  class DummyObjectData : public GraphicsObjectData {
   public:
    int PixelWidth(const GraphicsObject&) override { return 1; }
    int PixelHeight(const GraphicsObject&) override { return 1; }

    std::unique_ptr<GraphicsObjectData> Clone() const override {
      return std::make_unique<DummyObjectData>(*this);
    }

   protected:
    std::shared_ptr<const SDLSurface> CurrentSurface(
        const GraphicsObject&) override {
      return nullptr;
    }
  };

  void SetDummyData(GraphicsObject& object) {
    object.SetObjectData(std::make_unique<DummyObjectData>());
  }

  std::string GetExistFlags(const LazyArray<GraphicsObject>& la) {
    std::string ret;
    for (std::size_t i = 0, n = la.Size(); i < n; ++i)
      ret += la.Exists(i) ? '1' : '0';
    return ret;
  }
};

TEST_F(StageTest, ConstructorAndResetIncludeNextObjects) {
  Stage stage(3);
  stage.next_objects[1].Param().SetVisible(1);

  EXPECT_EQ(GetExistFlags(stage.next_objects), "010");
  stage.Reset();
  EXPECT_EQ(GetExistFlags(stage.next_objects), "000")
      << "Stage.Reset should clear next object buffer";
}

TEST_F(StageTest, WipePromotesBackToFrontAndClearsBack) {
  Stage stage(3);
  SetDummyData(stage.background_objects[0]);
  stage.background_objects[0].Param().SetVisible(1);
  stage.background_objects[0].Param().SetX(42);
  EXPECT_EQ(GetExistFlags(stage.background_objects), "100");

  stage.Wipe();

  EXPECT_EQ(GetExistFlags(stage.background_objects), "000");
  ASSERT_EQ(GetExistFlags(stage.foreground_objects), "100");
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_TRUE(stage.foreground_objects[0].Param().visible());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 42);
}

TEST_F(StageTest, WipeDoesNotAllocateEmptySlots) {
  Stage stage(3);

  stage.Wipe();

  EXPECT_EQ(GetExistFlags(stage.background_objects), "000");
  EXPECT_EQ(GetExistFlags(stage.foreground_objects), "000");
  EXPECT_EQ(GetExistFlags(stage.next_objects), "000");
}

TEST_F(StageTest, WipeOnlyAllocatesTouchedSlots) {
  Stage stage(3);
  SetDummyData(stage.background_objects[1]);
  stage.background_objects[1].Param().SetX(42);

  stage.Wipe();

  EXPECT_EQ(GetExistFlags(stage.foreground_objects), "010");
  EXPECT_EQ(GetExistFlags(stage.background_objects), "000");
  EXPECT_EQ(GetExistFlags(stage.next_objects), "000");
  EXPECT_EQ(stage.foreground_objects[1].Param().position_x, 42);
}

TEST_F(StageTest, WipePreservesOldFrontInNext) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  SetDummyData(stage.background_objects[0]);
  stage.background_objects[0].Param().SetX(20);

  stage.Wipe();

  ASSERT_EQ(GetExistFlags(stage.next_objects), "100");
  EXPECT_TRUE(stage.next_objects[0].has_object_data());
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 20);
}

TEST_F(StageTest, WipeCopyPreservesFrontWhenBackIsEmpty) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  stage.foreground_objects[0].Param().SetWipeCopy(1);

  stage.Wipe();

  ASSERT_EQ(GetExistFlags(stage.foreground_objects), "100");
  ASSERT_EQ(GetExistFlags(stage.next_objects), "100");
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 10);
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
}

TEST_F(StageTest, WipeRangeUsesLexicographicSorterComparison) {
  Stage stage(5);

  const int orders[] = {1, 1, 2, 2, 2};
  const int layers[] = {8, 9, 0, 1, 2};
  for (size_t i = 0; i < 5; ++i) {
    SetDummyData(stage.foreground_objects[i]);
    stage.foreground_objects[i].Param().z_order = orders[i];
    stage.foreground_objects[i].Param().z_layer = layers[i];
    stage.foreground_objects[i].Param().SetX(static_cast<int>(i + 10));
  }

  stage.Wipe(1, 2, 9, 1);

  ASSERT_EQ(GetExistFlags(stage.next_objects), "01110");
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_FALSE(stage.foreground_objects[1].has_object_data());
  EXPECT_FALSE(stage.foreground_objects[2].has_object_data());
  EXPECT_FALSE(stage.foreground_objects[3].has_object_data());
  EXPECT_TRUE(stage.foreground_objects[4].has_object_data());

  EXPECT_EQ(stage.next_objects[1].Param().position_x, 11);
  EXPECT_EQ(stage.next_objects[2].Param().position_x, 12);
  EXPECT_EQ(stage.next_objects[3].Param().position_x, 13);
}

TEST_F(StageTest, WipeRangeIgnoresEmptyAllocatedBackSlots) {
  Stage stage(1);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().z_order = 5;
  stage.foreground_objects[0].Param().SetX(10);
  stage.background_objects[0].Param().SetX(99);

  stage.Wipe(0, 0, std::numeric_limits<int>::min(),
             std::numeric_limits<int>::max());

  EXPECT_FALSE(stage.next_objects.Exists(0));
  ASSERT_TRUE(stage.foreground_objects.Exists(0));
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 10);
  ASSERT_TRUE(stage.background_objects.Exists(0));
  EXPECT_FALSE(stage.background_objects[0].has_object_data());
  EXPECT_EQ(stage.background_objects[0].Param().position_x, 99);
}

TEST_F(StageTest, BackWipeEraseClearsFrontAndConsumesBack) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  stage.background_objects[0].Param().SetWipeErase(1);

  stage.Wipe();

  ASSERT_EQ(GetExistFlags(stage.foreground_objects), "100");
  ASSERT_EQ(GetExistFlags(stage.next_objects), "100");
  ASSERT_EQ(GetExistFlags(stage.background_objects), "000");

  EXPECT_FALSE(stage.foreground_objects[0].has_object_data());
  EXPECT_EQ(stage.foreground_objects[0].Param().wipe_erase, 1);
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
}
