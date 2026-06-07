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
#include <stdexcept>
#include <vector>

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

  class RecordingObjectData : public DummyObjectData {
   public:
    RecordingObjectData(std::vector<int>* rendered, int id)
        : rendered_(rendered), id_(id) {}

    void Render(const GraphicsObject&, const GraphicsObject*) override {
      rendered_->push_back(id_);
    }

    std::unique_ptr<GraphicsObjectData> Clone() const override {
      return std::make_unique<RecordingObjectData>(*this);
    }

   private:
    std::vector<int>* rendered_;
    int id_;
  };

  void SetDummyData(GraphicsObject& object) {
    object.SetObjectData(std::make_unique<DummyObjectData>());
  }

  void SetRecordingData(GraphicsObject& object,
                        std::vector<int>* rendered,
                        int id) {
    object.SetObjectData(std::make_unique<RecordingObjectData>(rendered, id));
  }

  std::string GetExistFlags(const LazyArray<GraphicsObject>& la) {
    std::string ret;
    for (std::size_t i = 0, n = la.Size(); i < n; ++i)
      ret += la.Exists(i) ? '1' : '0';
    return ret;
  }
};

TEST_F(StageTest, ObjectLayerHelpersAccessExpectedBuffers) {
  Stage stage(3);

  EXPECT_EQ(&stage.GetObject(OBJ_FG, 0), &stage.foreground_objects[0]);
  EXPECT_EQ(&stage.GetObject(OBJ_BG, 1), &stage.background_objects[1]);
  EXPECT_EQ(&stage.GetObject(OBJ_NEXT, 2), &stage.next_objects[2]);

  EXPECT_THROW(stage.GetObject(-1, 0), std::runtime_error);
  EXPECT_THROW(stage.GetObject(3, 0), std::runtime_error);
  EXPECT_THROW(stage.GetFreeObjectId(3), std::runtime_error);
}

TEST_F(StageTest, SetRemoveAndFreeIdUseRequestedLayer) {
  Stage stage(3);
  stage.GetObject(OBJ_FG, 0).Param().SetX(7);
  EXPECT_EQ(stage.GetFreeObjectId(OBJ_FG), 1);

  GraphicsObject object;
  SetDummyData(object);
  object.Param().SetX(42);
  stage.SetObject(OBJ_NEXT, 2, std::move(object));

  ASSERT_TRUE(stage.next_objects.Exists(2));
  EXPECT_TRUE(stage.next_objects[2].has_object_data());
  EXPECT_EQ(stage.next_objects[2].Param().position_x, 42);

  stage.RemoveObject(OBJ_NEXT, 2);
  EXPECT_FALSE(stage.next_objects.Exists(2));
}

TEST_F(StageTest, FreeAndInitializeSingleObjectAffectFrontAndBackOnly) {
  Stage stage(1);
  SetDummyData(stage.foreground_objects[0]);
  SetDummyData(stage.background_objects[0]);
  SetDummyData(stage.next_objects[0]);
  stage.foreground_objects[0].Param().SetX(10);
  stage.background_objects[0].Param().SetX(20);
  stage.next_objects[0].Param().SetX(30);

  stage.FreeObjectData(0);

  EXPECT_FALSE(stage.foreground_objects[0].has_object_data());
  EXPECT_FALSE(stage.background_objects[0].has_object_data());
  EXPECT_TRUE(stage.next_objects[0].has_object_data());

  stage.InitializeObjectParams(0);

  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 0);
  EXPECT_EQ(stage.background_objects[0].Param().position_x, 0);
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 30);
}

TEST_F(StageTest, AnimationsPlayingIsFalseWithoutActiveForegroundAnimation) {
  Stage stage(1);
  EXPECT_FALSE(stage.AnimationsPlaying());

  SetDummyData(stage.foreground_objects[0]);
  EXPECT_FALSE(stage.AnimationsPlaying());
}

TEST_F(StageTest, RenderObjectsSortsFiltersAndReusesScratch) {
  Stage stage(4);
  std::vector<int> rendered;

  auto prepare = [&](int slot, int order, int layer, int depth) {
    GraphicsObject& object = stage.foreground_objects[slot];
    SetRecordingData(object, &rendered, slot);
    object.Param().SetVisible(1);
    object.Param().z_order = order;
    object.Param().z_layer = layer;
    object.Param().z_depth = depth;
  };

  prepare(0, 2, 0, 0);
  prepare(1, 1, 9, 0);
  prepare(2, 1, 1, 0);
  prepare(3, 3, 0, 0);

  stage.RenderObjects(
      [](size_t slot, const GraphicsObject&) { return slot != 1; });

  EXPECT_EQ(rendered, std::vector<int>({2, 0, 3}));

  rendered.clear();
  stage.RenderObjects(
      [](size_t slot, const GraphicsObject&) { return slot == 3; });

  EXPECT_EQ(rendered, std::vector<int>({3}));
}

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
