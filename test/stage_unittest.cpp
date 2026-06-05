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

#include <memory>

namespace {

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

}  // namespace

TEST(StageTest, ConstructorAndResetIncludeNextObjects) {
  Stage stage(3);
  stage.next_objects[1].Param().SetVisible(1);

  EXPECT_TRUE(stage.next_objects.Exists(1));
  stage.Reset();
  EXPECT_FALSE(stage.next_objects.Exists(1))
      << "Stage.Reset should clear next object buffer";
}

TEST(StageTest, WipePromotesBackToFrontAndClearsBack) {
  Stage stage(3);
  SetDummyData(stage.background_objects[0]);
  stage.background_objects[0].Param().SetVisible(1);
  stage.background_objects[0].Param().SetX(42);

  stage.Wipe();

  ASSERT_TRUE(stage.foreground_objects.Exists(0));
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_TRUE(stage.foreground_objects[0].Param().visible());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 42);
  EXPECT_FALSE(stage.background_objects.Exists(0));
}

TEST(StageTest, WipePreservesOldFrontInNext) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  SetDummyData(stage.background_objects[0]);
  stage.background_objects[0].Param().SetX(20);

  stage.Wipe();

  ASSERT_TRUE(stage.next_objects.Exists(0));
  EXPECT_TRUE(stage.next_objects[0].has_object_data());
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 20);
}

TEST(StageTest, WipeCopyPreservesFrontWhenBackIsEmpty) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  stage.foreground_objects[0].Param().SetWipeCopy(1);

  stage.Wipe();

  ASSERT_TRUE(stage.foreground_objects.Exists(0));
  EXPECT_TRUE(stage.foreground_objects[0].has_object_data());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 10);
  ASSERT_TRUE(stage.next_objects.Exists(0));
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
}

TEST(StageTest, BackWipeEraseClearsFrontAndConsumesBack) {
  Stage stage(3);
  SetDummyData(stage.foreground_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  stage.background_objects[0].Param().SetWipeErase(1);

  stage.Wipe();

  ASSERT_TRUE(stage.foreground_objects.Exists(0));
  EXPECT_FALSE(stage.foreground_objects[0].has_object_data());
  EXPECT_EQ(stage.foreground_objects[0].Param().wipe_erase, 1);
  EXPECT_FALSE(stage.background_objects.Exists(0));
  ASSERT_TRUE(stage.next_objects.Exists(0));
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
}
