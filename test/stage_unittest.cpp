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
#include "libsiglus/siglus_scene_renderer.hpp"

#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
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
        const GraphicsObject&) const override {
      return nullptr;
    }
  };

  class RecordingObjectData : public DummyObjectData {
   public:
    RecordingObjectData(std::vector<int>* rendered, int id)
        : rendered_(rendered), id_(id) {}

    void Render(const GraphicsObject&, std::optional<ParentObjState>) override {
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
    object.SetDrawer(std::make_unique<DummyObjectData>());
  }

  void SetRecordingData(GraphicsObject& object,
                        std::vector<int>* rendered,
                        int id) {
    object.SetDrawer(std::make_unique<RecordingObjectData>(rendered, id));
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

  EXPECT_EQ(&stage.GetObject(kLayerFg, 0), &stage.foreground_objects[0]);
  EXPECT_EQ(&stage.GetObject(kLayerBg, 1), &stage.background_objects[1]);
  EXPECT_EQ(&stage.GetObject(kLayerNext, 2), &stage.next_objects[2]);

  EXPECT_THROW(stage.GetObject(-1, 0), std::runtime_error);
  EXPECT_THROW(stage.GetObject(3, 0), std::runtime_error);
  EXPECT_THROW(stage.GetFreeObjectId(3), std::runtime_error);
}

TEST_F(StageTest, SetRemoveAndFreeIdUseRequestedLayer) {
  Stage stage(3);
  stage.GetObject(kLayerFg, 0).Param().SetX(7);
  EXPECT_EQ(stage.GetFreeObjectId(kLayerFg), 1);

  GraphicsObject object;
  SetDummyData(object);
  object.Param().SetX(42);
  stage.SetObject(kLayerNext, 2, std::move(object));

  ASSERT_TRUE(stage.next_objects.Exists(2));
  EXPECT_TRUE(stage.next_objects[2].HasDrawer());
  EXPECT_EQ(stage.next_objects[2].Param().position_x, 42);

  stage.RemoveObject(kLayerNext, 2);
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

  EXPECT_FALSE(stage.foreground_objects[0].HasDrawer());
  EXPECT_FALSE(stage.background_objects[0].HasDrawer());
  EXPECT_TRUE(stage.next_objects[0].HasDrawer());

  stage.InitializeObjectParams(0);

  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 0);
  EXPECT_EQ(stage.background_objects[0].Param().position_x, 0);
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 30);
}

TEST_F(StageTest, ConstructorAndResetIncludeNextObjects) {
  Stage stage(3);
  stage.next_objects[1].Param().SetVisible(1);
  stage.SetTransitionRenderAlpha(0.25, 0.75);

  EXPECT_EQ(GetExistFlags(stage.next_objects), "010");
  stage.Reset();
  EXPECT_EQ(GetExistFlags(stage.next_objects), "000")
      << "Stage.Reset should clear next object buffer";
  EXPECT_DOUBLE_EQ(stage.foreground_render_alpha(), 1.0);
  EXPECT_DOUBLE_EQ(stage.next_render_alpha(), 0.0);
}

TEST_F(StageTest, TransitionRenderAlphaIsClampedAndClearable) {
  Stage stage(1);

  EXPECT_DOUBLE_EQ(stage.foreground_render_alpha(), 1.0);
  EXPECT_DOUBLE_EQ(stage.next_render_alpha(), 0.0);

  stage.SetTransitionRenderAlpha(-0.5, 1.5);

  EXPECT_DOUBLE_EQ(stage.foreground_render_alpha(), 0.0);
  EXPECT_DOUBLE_EQ(stage.next_render_alpha(), 1.0);

  stage.ClearTransitionRenderState();

  EXPECT_DOUBLE_EQ(stage.foreground_render_alpha(), 1.0);
  EXPECT_DOUBLE_EQ(stage.next_render_alpha(), 0.0);
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
  EXPECT_TRUE(stage.foreground_objects[0].HasDrawer());
  EXPECT_TRUE(stage.foreground_objects[0].Param().visible());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 42);
}

TEST_F(StageTest, WipePromotesBackObjectWithOnlyChildren) {
  Stage stage(3);
  stage.background_objects[0].ResetChildren(1);
  stage.background_objects[0].Param().SetVisible(1);
  stage.background_objects[0].Param().SetX(42);
  GraphicsObject& child = stage.background_objects[0].TouchChild(0);
  SetDummyData(child);
  child.Param().SetX(7);

  stage.Wipe();

  EXPECT_EQ(GetExistFlags(stage.background_objects), "000");
  ASSERT_EQ(GetExistFlags(stage.foreground_objects), "100");
  EXPECT_FALSE(stage.foreground_objects[0].HasDrawer());
  ASSERT_TRUE(stage.foreground_objects[0].HasChildren());
  ASSERT_NE(stage.foreground_objects[0].GetChild(0), nullptr);
  EXPECT_TRUE(stage.foreground_objects[0].GetChild(0)->HasDrawer());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 42);
  EXPECT_EQ(stage.foreground_objects[0].GetChild(0)->Param().position_x, 7);
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
  EXPECT_TRUE(stage.next_objects[0].HasDrawer());
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
  EXPECT_TRUE(stage.foreground_objects[0].HasDrawer());
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
  EXPECT_TRUE(stage.foreground_objects[0].HasDrawer());
  EXPECT_FALSE(stage.foreground_objects[1].HasDrawer());
  EXPECT_FALSE(stage.foreground_objects[2].HasDrawer());
  EXPECT_FALSE(stage.foreground_objects[3].HasDrawer());
  EXPECT_TRUE(stage.foreground_objects[4].HasDrawer());

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
  EXPECT_TRUE(stage.foreground_objects[0].HasDrawer());
  EXPECT_EQ(stage.foreground_objects[0].Param().position_x, 10);
  ASSERT_TRUE(stage.background_objects.Exists(0));
  EXPECT_FALSE(stage.background_objects[0].HasDrawer());
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

  EXPECT_FALSE(stage.foreground_objects[0].HasDrawer());
  EXPECT_EQ(stage.foreground_objects[0].Param().wipe_erase, 1);
  EXPECT_EQ(stage.next_objects[0].Param().position_x, 10);
}

TEST_F(StageTest, WipePromotesGroupsInRangeAndClearsTransientState) {
  Stage stage(1);
  stage.groups[kLayerFg].resize(3);
  stage.groups[kLayerBg].resize(3);
  stage.groups[kLayerNext].resize(4);

  Group& before = stage.groups[kLayerFg][1];
  before.order = 1;
  before.layer = 9;
  before.cancel_priority = 11;
  before.status = Group::Status::Active;
  before.result = Group::Result::Decided;
  before.result_button_no = 101;
  before.hit_button_no = 1;
  before.pushed_button_no = 1;
  before.pressed_button_no = 1;

  Group& replacement = stage.groups[kLayerBg][1];
  replacement.order = 7;
  replacement.layer = 8;
  replacement.cancel_priority = 22;
  replacement.status = Group::Status::Waiting;
  replacement.result = Group::Result::Canceled;
  replacement.result_button_no = -1;
  replacement.hit_button_no = 2;
  replacement.pressed_button_no = 2;

  stage.groups[kLayerFg][0].order = 1;
  stage.groups[kLayerFg][0].layer = 8;
  stage.groups[kLayerFg][2].order = 2;
  stage.groups[kLayerFg][2].layer = 2;
  stage.groups[kLayerBg][0].cancel_priority = 30;
  stage.groups[kLayerBg][2].cancel_priority = 40;

  stage.Wipe(1, 2, 9, 1);

  ASSERT_EQ(stage.groups[kLayerNext].size(), 3);
  const Group& saved = stage.groups[kLayerNext][1];
  EXPECT_EQ(saved.order, 1);
  EXPECT_EQ(saved.layer, 9);
  EXPECT_EQ(saved.cancel_priority, 11);
  EXPECT_EQ(saved.status, Group::Status::Active);
  EXPECT_EQ(saved.result, Group::Result::Decided);
  EXPECT_EQ(saved.result_button_no, 101);
  EXPECT_FALSE(saved.hit_button_no);
  EXPECT_FALSE(saved.pushed_button_no);
  EXPECT_FALSE(saved.pressed_button_no);

  const Group& promoted = stage.groups[kLayerFg][1];
  EXPECT_EQ(promoted.order, 7);
  EXPECT_EQ(promoted.layer, 8);
  EXPECT_EQ(promoted.cancel_priority, 22);
  EXPECT_EQ(promoted.status, Group::Status::Waiting);
  EXPECT_EQ(promoted.result, Group::Result::Canceled);
  EXPECT_EQ(promoted.result_button_no, -1);
  EXPECT_FALSE(promoted.hit_button_no);
  EXPECT_FALSE(promoted.pressed_button_no);

  const Group& reset = stage.groups[kLayerBg][1];
  EXPECT_EQ(reset.order, 0);
  EXPECT_EQ(reset.layer, 0);
  EXPECT_EQ(reset.cancel_priority, 0);
  EXPECT_EQ(reset.status, Group::Status::Disabled);
  EXPECT_EQ(reset.result, Group::Result::None);

  EXPECT_EQ(stage.groups[kLayerFg][0].order, 1);
  EXPECT_EQ(stage.groups[kLayerBg][0].cancel_priority, 30);
  EXPECT_EQ(stage.groups[kLayerFg][2].layer, 2);
  EXPECT_EQ(stage.groups[kLayerBg][2].cancel_priority, 40);
}

TEST_F(StageTest, FullWipePromotesSparseBackgroundGroups) {
  Stage stage(1);
  stage.groups[kLayerBg].resize(4);
  Group& background = stage.groups[kLayerBg][3];
  background.order = 5;
  background.layer = 6;
  background.cancel_priority = 42;
  background.status = Group::Status::Active;
  background.result = Group::Result::Decided;
  background.result_button_no = 8;

  stage.Wipe();

  ASSERT_EQ(stage.groups[kLayerFg].size(), 4);
  ASSERT_EQ(stage.groups[kLayerBg].size(), 4);
  ASSERT_EQ(stage.groups[kLayerNext].size(), 4);
  EXPECT_EQ(stage.groups[kLayerFg][3].order, 5);
  EXPECT_EQ(stage.groups[kLayerFg][3].layer, 6);
  EXPECT_EQ(stage.groups[kLayerFg][3].cancel_priority, 42);
  EXPECT_EQ(stage.groups[kLayerFg][3].status, Group::Status::Active);
  EXPECT_EQ(stage.groups[kLayerFg][3].result_button_no, 8);
  EXPECT_EQ(stage.groups[kLayerBg][3].status, Group::Status::Disabled);
  EXPECT_EQ(stage.groups[kLayerNext][3].status, Group::Status::Disabled);
}
