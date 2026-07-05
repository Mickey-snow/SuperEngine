// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "core/object.hpp"
#include "core/object_internal/drawer/file.hpp"
#include "core/object_internal/objdrawer.hpp"
#include "systems/sdl/sdl_surface.hpp"

namespace {

class TestGraphicsObjectData : public GraphicsObjectData {
 public:
  TestGraphicsObjectData(Rect src,
                         Point texture_origin,
                         std::shared_ptr<SDLSurface> surface)
      : src_(src),
        texture_origin_(texture_origin),
        surface_(std::move(surface)) {}

  int PixelWidth(const GraphicsObject& go) override { return src_.width(); }
  int PixelHeight(const GraphicsObject& go) override { return src_.height(); }

  std::unique_ptr<GraphicsObjectData> Clone() const override {
    return std::make_unique<TestGraphicsObjectData>(*this);
  }

 protected:
  std::shared_ptr<const SDLSurface> CurrentSurface(
      const GraphicsObject& go) const override {
    return surface_;
  }

  Rect SrcRect(const GraphicsObject& go) const override { return src_; }

  Point DstOrigin(const GraphicsObject& go) const override {
    return texture_origin_;
  }

 private:
  Rect src_;
  Point texture_origin_;
  std::shared_ptr<SDLSurface> surface_;
};

std::shared_ptr<SDLSurface> OpaqueSurface(Size size) {
  auto surface = std::make_shared<SDLSurface>(size);
  surface->Fill(RGBAColour(255, 255, 255, 255));
  return surface;
}

TestGraphicsObjectData& SetTestData(GraphicsObject& object,
                                    Rect src,
                                    Point texture_origin,
                                    std::shared_ptr<SDLSurface> surface) {
  auto data =
      std::make_unique<TestGraphicsObjectData>(src, texture_origin, surface);
  auto& ref = *data;
  object.SetDrawer(std::move(data));
  return ref;
}

}  // namespace

TEST(ObjectDrawerTest, DestinationAddsObjectCenterAndTextureOrigin) {
  auto surface = OpaqueSurface(Size(128, 32));
  GraphicsObject object;
  auto& data =
      SetTestData(object, Rect::REC(0, 0, 101, 20), Point(5, 3), surface);
  object.Param().SetX(100);
  object.Param().SetY(200);
  object.Param().SetOriginX(10);
  object.Param().SetOriginY(7);

  EXPECT_EQ(data.DstRect(object, nullptr), Rect::GRP(85, 190, 186, 210));
}

TEST(ObjectDrawerTest, DestinationScalesAroundLegacyAnchor) {
  auto surface = OpaqueSurface(Size(128, 64));
  GraphicsObject object;
  auto& data =
      SetTestData(object, Rect::REC(0, 0, 100, 40), Point(5, 0), surface);
  object.Param().SetX(100);
  object.Param().SetY(50);
  object.Param().SetOriginX(10);
  object.Param().SetRepOriginX(20);
  object.Param().SetHqScaleX(500);

  EXPECT_EQ(data.DstRect(object, nullptr), Rect::GRP(102, 50, 152, 90));
}

TEST(ObjectDrawerTest, ParentScaleAndPositionAffectChildPositionAndSize) {
  auto surface = OpaqueSurface(Size(32, 32));
  GraphicsObject parent;
  SetTestData(parent, Rect::REC(0, 0, 10, 10), Point(), surface);
  parent.Param().SetX(100);
  parent.Param().SetY(50);
  parent.Param().SetRepOriginX(10);
  parent.Param().SetRepOriginY(20);
  parent.Param().SetHqScaleX(2000);
  parent.Param().SetHqScaleY(2000);

  GraphicsObject child;
  auto& data = SetTestData(child, Rect::REC(0, 0, 10, 10), Point(), surface);
  child.Param().SetX(15);
  child.Param().SetY(25);

  EXPECT_EQ(data.DstRect(child, &parent), Rect::GRP(120, 80, 140, 100));
}

TEST(ObjectDrawerTest, ParentRotationAffectsChildPosition) {
  auto surface = OpaqueSurface(Size(32, 32));
  GraphicsObject parent;
  SetTestData(parent, Rect::REC(0, 0, 10, 10), Point(), surface);
  parent.Param().SetX(100);
  parent.Param().SetY(50);
  parent.Param().SetRepOriginX(10);
  parent.Param().SetRepOriginY(20);
  parent.Param().SetRotation(900);

  GraphicsObject child;
  auto& data = SetTestData(child, Rect::REC(0, 0, 10, 10), Point(), surface);
  child.Param().SetX(20);
  child.Param().SetY(20);

  EXPECT_EQ(data.DstRect(child, &parent), Rect::GRP(110, 80, 120, 90));
}

TEST(ObjectDrawerTest, ButtonOffsetsContributeToGeometry) {
  auto surface = OpaqueSurface(Size(32, 32));
  GraphicsObject object;
  auto& data = SetTestData(object, Rect::REC(0, 0, 10, 10), Point(), surface);
  object.Param().SetX(10);
  object.Param().SetY(20);
  object.Param().SetButtonOverrides(0, 8, -4);

  EXPECT_EQ(data.DstRect(object, nullptr), Rect::GRP(18, 16, 28, 26));
  EXPECT_TRUE(data.HitTest(object, Point(20, 20)));
  EXPECT_FALSE(data.HitTest(object, Point(12, 20)));
}

TEST(ObjectDrawerTest, ParentButtonOffsetsContributeToChildGeometry) {
  auto surface = OpaqueSurface(Size(32, 32));
  GraphicsObject parent;
  SetTestData(parent, Rect::REC(0, 0, 10, 10), Point(), surface);
  parent.Param().SetX(10);
  parent.Param().SetY(20);
  parent.Param().SetButtonOverrides(0, 8, -4);

  GraphicsObject child;
  auto& data = SetTestData(child, Rect::REC(0, 0, 10, 10), Point(), surface);
  child.Param().SetX(5);
  child.Param().SetY(6);

  EXPECT_EQ(data.DstRect(child, &parent), Rect::GRP(23, 22, 33, 32));
  ParentObjState parent_state = ParentObjState::BuildFrom(parent);
  EXPECT_TRUE(data.HitTest(child, Point(25, 25), parent_state));
  EXPECT_FALSE(data.HitTest(child, Point(17, 25), parent_state));
}

TEST(ObjectDrawerTest, HitTestUsesRotationAroundLegacyPivot) {
  auto surface = OpaqueSurface(Size(32, 32));
  GraphicsObject object;
  auto& data = SetTestData(object, Rect::REC(0, 0, 10, 20), Point(), surface);
  object.Param().SetX(100);
  object.Param().SetY(100);
  object.Param().SetRotation(900);

  EXPECT_TRUE(data.HitTest(object, Point(95, 105)));
  EXPECT_FALSE(data.HitTest(object, Point(105, 105)));
}

TEST(ObjectDrawerTest, HitTestSamplesAlphaWhenAlphaTestIsEnabled) {
  auto surface = std::make_shared<SDLSurface>(Size(10, 10));
  surface->Fill(RGBAColour(255, 255, 255, 0));
  surface->Fill(RGBAColour(255, 255, 255, 255), Rect::REC(5, 0, 5, 10));

  GraphicsObject object;
  auto& data = SetTestData(object, Rect::REC(0, 0, 10, 10), Point(), surface);

  EXPECT_FALSE(data.HitTest(object, Point(2, 5)));
  EXPECT_TRUE(data.HitTest(object, Point(7, 5)));
}

TEST(ObjectDrawerTest, CompositeObjectBoundsUnionLayerRectangles) {
  std::vector<CompositeGraphicsObjectLayer> layers = {
      {.surface = OpaqueSurface(Size(10, 10)),
       .offset = Point(0, 0),
       .cut_no = 0,
       .blend_type = 0},
      {.surface = OpaqueSurface(Size(5, 6)),
       .offset = Point(8, -2),
       .cut_no = 0,
       .blend_type = 0},
  };

  GraphicsObject object;
  auto data = std::make_unique<CompositeGraphicsObject>(std::move(layers));
  auto& ref = *data;
  object.SetDrawer(std::move(data));
  object.Param().SetX(100);
  object.Param().SetY(50);

  EXPECT_EQ(object.PixelWidth(), 13);
  EXPECT_EQ(object.PixelHeight(), 12);
  EXPECT_EQ(ref.DstRect(object, nullptr), Rect::GRP(100, 48, 113, 60));
}

TEST(ObjectDrawerTest, CompositeObjectHitTestUsesComposedAlpha) {
  auto left = std::make_shared<SDLSurface>(Size(4, 4));
  left->Fill(RGBAColour(255, 255, 255, 255));
  auto right = std::make_shared<SDLSurface>(Size(4, 4));
  right->Fill(RGBAColour(255, 255, 255, 255));

  std::vector<CompositeGraphicsObjectLayer> layers = {
      {.surface = left, .offset = Point(0, 0), .cut_no = 0, .blend_type = 0},
      {.surface = right, .offset = Point(10, 0), .cut_no = 0, .blend_type = 0},
  };

  GraphicsObject object;
  auto data = std::make_unique<CompositeGraphicsObject>(std::move(layers));
  auto& ref = *data;
  object.SetDrawer(std::move(data));

  EXPECT_TRUE(ref.HitTest(object, Point(2, 2)));
  EXPECT_FALSE(ref.HitTest(object, Point(8, 2)));
  EXPECT_TRUE(ref.HitTest(object, Point(11, 2)));
}

TEST(ObjectDrawerTest, CompositeObjectClonePreservesLayerData) {
  std::vector<CompositeGraphicsObjectLayer> layers = {
      {.surface = OpaqueSurface(Size(3, 3)),
       .offset = Point(0, 0),
       .cut_no = 0,
       .blend_type = 0},
      {.surface = OpaqueSurface(Size(2, 2)),
       .offset = Point(5, 0),
       .cut_no = 0,
       .blend_type = 0},
  };

  GraphicsObject object;
  object.SetDrawer(
      std::make_unique<CompositeGraphicsObject>(std::move(layers)));

  GraphicsObject cloned = object.Clone();
  EXPECT_EQ(cloned.PixelWidth(), 7);
  EXPECT_EQ(cloned.PixelHeight(), 3);
  EXPECT_TRUE(cloned.GetDrawer().HitTest(cloned, Point(1, 1)));
  EXPECT_TRUE(cloned.GetDrawer().HitTest(cloned, Point(6, 1)));
}
