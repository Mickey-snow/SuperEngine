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
#include "core/object_internal/objdrawer.hpp"
#include "core/object_internal/object_mutator.hpp"

#include <memory>
#include <optional>

class GraphicsObjectTest : public ::testing::Test {
 protected:
  class DummyObjectData : public GraphicsObjectData {
   public:
    DummyObjectData(int* render_count = nullptr,
                    std::optional<ParentObjState>* last_parent = nullptr)
        : render_count_(render_count), last_parent_(last_parent) {}

    void Render(const GraphicsObject&,
                std::optional<ParentObjState> parent) override {
      if (render_count_)
        ++*render_count_;
      if (last_parent_)
        *last_parent_ = parent;
    }

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

   private:
    int* render_count_;
    std::optional<ParentObjState>* last_parent_;
  };

  void SetUp() override {
    auto& param = obj.Param();

    param.is_visible = true;
    param.position_x = 10;
    param.position_y = 20;
  }

  void SetDummyData(GraphicsObject& object) {
    object.SetDrawer(std::make_unique<DummyObjectData>());
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

TEST_F(GraphicsObjectTest, CloneCopiesFilePath) {
  obj.SetFilePath("bg47");

  GraphicsObject other = obj.Clone();

  EXPECT_EQ(other.FilePath(), "bg47");
  other.SetFilePath("cg01");
  EXPECT_EQ(obj.FilePath(), "bg47");
}

TEST_F(GraphicsObjectTest, MoveTransfersFilePath) {
  obj.SetFilePath("bg47");

  GraphicsObject other = std::move(obj);

  EXPECT_EQ(other.FilePath(), "bg47");
  EXPECT_TRUE(obj.FilePath().empty());

  GraphicsObject assigned;
  assigned = std::move(other);
  EXPECT_EQ(assigned.FilePath(), "bg47");
  EXPECT_TRUE(other.FilePath().empty());
}

TEST_F(GraphicsObjectTest, CloneCopiesChildrenDeeply) {
  obj.ResetChildren(2);
  GraphicsObject& child = obj.TouchChild(1);
  child.Param().SetX(42);
  SetDummyData(child);

  GraphicsObject other = obj.Clone();

  ASSERT_TRUE(other.HasChildren());
  ASSERT_NE(other.GetChild(1), nullptr);
  ASSERT_NE(obj.GetChild(1), other.GetChild(1));
  EXPECT_EQ(other.GetChild(1)->Param().position_x, 42);
  EXPECT_TRUE(other.GetChild(1)->HasDrawer());

  other.GetChild(1)->Param().SetX(100);
  EXPECT_EQ(obj.GetChild(1)->Param().position_x, 42);
}

TEST_F(GraphicsObjectTest, SetObjectDataClearsChildren) {
  obj.ResetChildren(1);
  obj.TouchChild(0).Param().SetX(42);

  SetDummyData(obj);

  EXPECT_TRUE(obj.HasDrawer());
  EXPECT_FALSE(obj.HasChildren());
}

TEST_F(GraphicsObjectTest, ResetChildrenClearsFilePath) {
  obj.SetFilePath("bg47");

  obj.ResetChildren(1);

  EXPECT_TRUE(obj.FilePath().empty());
}

TEST_F(GraphicsObjectTest, InitializeParamsPreservesChildren) {
  obj.ResetChildren(1);
  obj.TouchChild(0).Param().SetX(42);
  obj.SetFilePath("bg47");

  obj.InitializeParams();

  ASSERT_TRUE(obj.HasChildren());
  ASSERT_NE(obj.GetChild(0), nullptr);
  EXPECT_EQ(obj.GetChild(0)->Param().position_x, 42);
  EXPECT_EQ(obj.FilePath(), "bg47");
}

TEST_F(GraphicsObjectTest, FreeObjectDataClearsChildren) {
  obj.ResetChildren(1);
  obj.TouchChild(0);
  obj.SetFilePath("bg47");

  obj.FreeObjectData();

  EXPECT_FALSE(obj.HasDrawer());
  EXPECT_FALSE(obj.HasChildren());
  EXPECT_TRUE(obj.FilePath().empty());
}

TEST_F(GraphicsObjectTest, FreeDataAndInitializeParamsClearsChildren) {
  obj.ResetChildren(1);
  obj.TouchChild(0);
  obj.Param().SetX(42);
  obj.SetFilePath("bg47");

  obj.FreeDataAndInitializeParams();

  EXPECT_FALSE(obj.HasChildren());
  EXPECT_EQ(obj.Param().position_x, 0);
  EXPECT_TRUE(obj.FilePath().empty());
}

TEST_F(GraphicsObjectTest, InvisibleParentDoesNotRenderChildren) {
  obj.ResetChildren(1);
  obj.Param().SetVisible(false);
  GraphicsObject& child = obj.TouchChild(0);
  child.Param().SetVisible(true);
  int render_count = 0;
  child.SetDrawer(std::make_unique<DummyObjectData>(&render_count));

  obj.Render();

  EXPECT_EQ(render_count, 0);
}

TEST_F(GraphicsObjectTest, ParentWithoutObjectDataPassesStateToChildren) {
  obj.ResetChildren(1);
  obj.Param().SetVisible(true);
  obj.Param().SetX(10);
  obj.Param().SetY(20);
  obj.Param().SetAlpha(128);
  GraphicsObject& child = obj.TouchChild(0);
  child.Param().SetVisible(true);

  int render_count = 0;
  std::optional<ParentObjState> last_parent;
  child.SetDrawer(
      std::make_unique<DummyObjectData>(&render_count, &last_parent));

  obj.Render();

  EXPECT_EQ(render_count, 1);
  ASSERT_TRUE(last_parent.has_value());
  EXPECT_FLOAT_EQ(last_parent->render_state.pos_x, 10.0f);
  EXPECT_FLOAT_EQ(last_parent->render_state.pos_y, 20.0f);
  EXPECT_FLOAT_EQ(last_parent->alpha, obj.Param().GetNormalizedAlpha());
}

TEST_F(GraphicsObjectTest, EndObjectMutatorMatching) {
  obj.AddObjectMutator(ObjectMutator({}, -1, "fade"));
  EXPECT_TRUE(obj.IsMutatorRunningMatching(-1, "fade"));

  obj.EndObjectMutatorMatching(-1, "fade", 0);
  EXPECT_FALSE(obj.IsMutatorRunningMatching(-1, "fade"));
}
