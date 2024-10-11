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

#include "object/parameter_manager.h"
#include "systems/base/graphics_object.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

class GraphicsObjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& param = obj.Param();

    // uncategorized basic properties
    param.SetVert(1);
    param.SetOriginX(25);
    param.SetOriginY(30);
    param.SetRepOriginX(15);
    param.SetRepOriginY(20);
    param.SetWidth(80);
    param.SetHeight(90);
    param.SetHqWidth(800);
    param.SetHqHeight(900);
    param.SetRotation(45);
    param.SetPattNo(5);
    param.SetMono(1);
    param.SetInvert(1);
    param.SetLight(1);
    RGBColour tint(100, 150, 200);
    param.SetTint(tint);
    RGBAColour colour(50, 60, 70, 80);
    param.SetColour(colour);
    Rect clip_rect = Rect::GRP(0, 0, 100, 100);
    param.SetClipRect(clip_rect);

    // text properties
    param.SetTextText("Hello World");
    param.SetTextOps(12, 2, 3, 5, 255, 128);

    // drift properties
    Rect drift_area = Rect::GRP(0, 0, 100, 100);
    param.SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1, drift_area);

    // digit properties
    param.SetDigitValue(12345);
    param.SetDigitOpts(5, 1, 1, 0, 2);

    // button properties
    param.SetButtonOpts(1, 10, 2, 3);
    param.SetButtonState(1);
    param.SetButtonOverrides(5, 10, 15);
  }

  GraphicsObject obj, default_obj;
};

TEST_F(GraphicsObjectTest, DefaultBasicProperty) {
  auto& default_param = default_obj.Param();

  EXPECT_EQ(default_param.visible(), false);
  EXPECT_EQ(default_param.x(), 0);
  EXPECT_EQ(default_param.y(), 0);
  EXPECT_EQ(default_param.GetXAdjustmentSum(), 0);
  EXPECT_EQ(default_param.GetYAdjustmentSum(), 0);
  EXPECT_EQ(default_param.vert(), 0);
  EXPECT_EQ(default_param.origin_x(), 0);
  EXPECT_EQ(default_param.origin_y(), 0);
  EXPECT_EQ(default_param.rep_origin_x(), 0);
  EXPECT_EQ(default_param.rep_origin_y(), 0);
  EXPECT_EQ(default_param.width(), 100);
  EXPECT_EQ(default_param.height(), 100);
  EXPECT_EQ(default_param.hq_width(), 1000);
  EXPECT_EQ(default_param.hq_height(), 1000);
  EXPECT_EQ(default_param.rotation(), 0);
  EXPECT_EQ(default_param.GetPattNo(), 0);
  EXPECT_EQ(default_param.mono(), 0);
  EXPECT_EQ(default_param.invert(), 0);
  EXPECT_EQ(default_param.light(), 0);
  EXPECT_EQ(default_param.tint(), RGBColour(0, 0, 0));
  EXPECT_EQ(default_param.colour(), RGBAColour(0, 0, 0, 0));
  EXPECT_EQ(default_param.composite_mode(), 0);
  EXPECT_EQ(default_param.scroll_rate_x(), 0);
  EXPECT_EQ(default_param.scroll_rate_y(), 0);
  EXPECT_EQ(default_param.z_order(), 0);
  EXPECT_EQ(default_param.z_layer(), 0);
  EXPECT_EQ(default_param.z_depth(), 0);
  EXPECT_EQ(default_param.raw_alpha(), 255);
  EXPECT_FALSE(default_param.has_clip_rect());
  EXPECT_EQ(default_param.wipe_copy(), 0);
}

TEST_F(GraphicsObjectTest, SetGetBasicProperties) {
  default_obj.Param().SetVisible(true);
  EXPECT_EQ(default_obj.Param().visible(), true);

  default_obj.Param().SetX(50);
  EXPECT_EQ(default_obj.Param().x(), 50);

  default_obj.Param().SetY(100);
  EXPECT_EQ(default_obj.Param().y(), 100);

  default_obj.Param().SetXAdjustment(0, 5);
  EXPECT_EQ(default_obj.Param().x_adjustment(0), 5);
  EXPECT_EQ(default_obj.Param().GetXAdjustmentSum(), 5);

  default_obj.Param().SetYAdjustment(0, -10);
  EXPECT_EQ(default_obj.Param().y_adjustment(0), -10);
  EXPECT_EQ(default_obj.Param().GetYAdjustmentSum(), -10);

  default_obj.Param().SetVert(1);
  EXPECT_EQ(default_obj.Param().vert(), 1);

  default_obj.Param().SetOriginX(25);
  EXPECT_EQ(default_obj.Param().origin_x(), 25);

  default_obj.Param().SetOriginY(30);
  EXPECT_EQ(default_obj.Param().origin_y(), 30);

  default_obj.Param().SetRepOriginX(15);
  EXPECT_EQ(default_obj.Param().rep_origin_x(), 15);

  default_obj.Param().SetRepOriginY(20);
  EXPECT_EQ(default_obj.Param().rep_origin_y(), 20);

  default_obj.Param().SetWidth(80);
  EXPECT_EQ(default_obj.Param().width(), 80);

  default_obj.Param().SetHeight(90);
  EXPECT_EQ(default_obj.Param().height(), 90);

  default_obj.Param().SetHqWidth(800);
  EXPECT_EQ(default_obj.Param().hq_width(), 800);

  default_obj.Param().SetHqHeight(900);
  EXPECT_EQ(default_obj.Param().hq_height(), 900);

  default_obj.Param().SetRotation(45);
  EXPECT_EQ(default_obj.Param().rotation(), 45);

  default_obj.Param().SetPattNo(5);
  EXPECT_EQ(default_obj.Param().GetPattNo(), 5);

  default_obj.Param().SetMono(1);
  EXPECT_EQ(default_obj.Param().mono(), 1);

  default_obj.Param().SetInvert(1);
  EXPECT_EQ(default_obj.Param().invert(), 1);

  default_obj.Param().SetLight(1);
  EXPECT_EQ(default_obj.Param().light(), 1);

  RGBColour tint(100, 150, 200);
  default_obj.Param().SetTint(tint);
  EXPECT_EQ(default_obj.Param().tint(), tint);
  EXPECT_EQ(default_obj.Param().tint_red(), 100);
  EXPECT_EQ(default_obj.Param().tint_green(), 150);
  EXPECT_EQ(default_obj.Param().tint_blue(), 200);

  default_obj.Param().SetTintRed(110);
  EXPECT_EQ(default_obj.Param().tint_red(), 110);

  default_obj.Param().SetTintGreen(160);
  EXPECT_EQ(default_obj.Param().tint_green(), 160);

  default_obj.Param().SetTintBlue(210);
  EXPECT_EQ(default_obj.Param().tint_blue(), 210);

  RGBAColour colour(50, 60, 70, 80);
  default_obj.Param().SetColour(colour);
  EXPECT_EQ(default_obj.Param().colour(), colour);
  EXPECT_EQ(default_obj.Param().colour_red(), 50);
  EXPECT_EQ(default_obj.Param().colour_green(), 60);
  EXPECT_EQ(default_obj.Param().colour_blue(), 70);
  EXPECT_EQ(default_obj.Param().colour_level(), 80);

  default_obj.Param().SetColourRed(55);
  EXPECT_EQ(default_obj.Param().colour_red(), 55);

  default_obj.Param().SetColourGreen(65);
  EXPECT_EQ(default_obj.Param().colour_green(), 65);

  default_obj.Param().SetColourBlue(75);
  EXPECT_EQ(default_obj.Param().colour_blue(), 75);

  default_obj.Param().SetColourLevel(85);
  EXPECT_EQ(default_obj.Param().colour_level(), 85);

  default_obj.Param().SetCompositeMode(2);
  EXPECT_EQ(default_obj.Param().composite_mode(), 2);

  default_obj.Param().SetScrollRateX(5);
  EXPECT_EQ(default_obj.Param().scroll_rate_x(), 5);

  default_obj.Param().SetScrollRateY(-5);
  EXPECT_EQ(default_obj.Param().scroll_rate_y(), -5);

  default_obj.Param().SetZOrder(1);
  EXPECT_EQ(default_obj.Param().z_order(), 1);

  default_obj.Param().SetZLayer(2);
  EXPECT_EQ(default_obj.Param().z_layer(), 2);

  default_obj.Param().SetZDepth(3);
  EXPECT_EQ(default_obj.Param().z_depth(), 3);

  default_obj.Param().SetAlpha(128);
  EXPECT_EQ(default_obj.Param().raw_alpha(), 128);

  default_obj.Param().SetAlphaAdjustment(0, 10);
  EXPECT_EQ(default_obj.Param().alpha_adjustment(0), 10);

  Rect clip_rect = Rect::GRP(0, 0, 100, 100);
  default_obj.Param().SetClipRect(clip_rect);
  EXPECT_TRUE(default_obj.Param().has_clip_rect());
  EXPECT_EQ(default_obj.Param().clip_rect(), clip_rect);

  default_obj.Param().ClearClipRect();
  EXPECT_FALSE(default_obj.Param().has_clip_rect());

  Rect own_clip_rect = Rect::GRP(10, 10, 80, 80);
  default_obj.Param().SetOwnClipRect(own_clip_rect);
  EXPECT_TRUE(default_obj.Param().has_own_clip_rect());
  EXPECT_EQ(default_obj.Param().own_clip_rect(), own_clip_rect);

  default_obj.Param().ClearOwnClipRect();
  EXPECT_FALSE(default_obj.Param().has_own_clip_rect());

  default_obj.Param().SetWipeCopy(1);
  EXPECT_EQ(default_obj.Param().wipe_copy(), 1);
}

TEST_F(GraphicsObjectTest, TextProperties) {
  default_obj.Param().SetTextText("Hello World");
  EXPECT_EQ(default_obj.Param().GetTextText(), "Hello World");

  default_obj.Param().SetTextOps(12, 2, 3, 5, 255, 128);
  EXPECT_EQ(default_obj.Param().GetTextSize(), 12);
  EXPECT_EQ(default_obj.Param().GetTextXSpace(), 2);
  EXPECT_EQ(default_obj.Param().GetTextYSpace(), 3);
  EXPECT_EQ(default_obj.Param().GetTextCharCount(), 5);
  EXPECT_EQ(default_obj.Param().GetTextColour(), 255);
  EXPECT_EQ(default_obj.Param().GetTextShadowColour(), 128);
}

TEST_F(GraphicsObjectTest, DriftProperties) {
  Rect drift_area = Rect::GRP(0, 0, 100, 100);
  default_obj.Param().SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1,
                                   drift_area);

  EXPECT_EQ(default_obj.Param().GetDriftParticleCount(), 10);
  EXPECT_EQ(default_obj.Param().GetDriftUseAnimation(), 1);
  EXPECT_EQ(default_obj.Param().GetDriftStartPattern(), 0);
  EXPECT_EQ(default_obj.Param().GetDriftEndPattern(), 5);
  EXPECT_EQ(default_obj.Param().GetDriftAnimationTime(), 1000);
  EXPECT_EQ(default_obj.Param().GetDriftYSpeed(), 2);
  EXPECT_EQ(default_obj.Param().GetDriftPeriod(), 50);
  EXPECT_EQ(default_obj.Param().GetDriftAmplitude(), 10);
  EXPECT_EQ(default_obj.Param().GetDriftUseDrift(), 1);
  EXPECT_EQ(default_obj.Param().GetDriftUnknown(), 0);
  EXPECT_EQ(default_obj.Param().GetDriftDriftSpeed(), 1);
  EXPECT_EQ(default_obj.Param().GetDriftArea(), drift_area);
}

TEST_F(GraphicsObjectTest, DigitProperties) {
  default_obj.Param().SetDigitValue(12345);
  EXPECT_EQ(default_obj.Param().GetDigitValue(), 12345);

  default_obj.Param().SetDigitOpts(5, 1, 1, 0, 2);
  EXPECT_EQ(default_obj.Param().GetDigitDigits(), 5);
  EXPECT_EQ(default_obj.Param().GetDigitZero(), 1);
  EXPECT_EQ(default_obj.Param().GetDigitSign(), 1);
  EXPECT_EQ(default_obj.Param().GetDigitPack(), 0);
  EXPECT_EQ(default_obj.Param().GetDigitSpace(), 2);
}

TEST_F(GraphicsObjectTest, ButtonProperties) {
  default_obj.Param().SetButtonOpts(1, 10, 2, 3);
  EXPECT_EQ(default_obj.Param().IsButton(), 1);
  EXPECT_EQ(default_obj.Param().GetButtonAction(), 1);
  EXPECT_EQ(default_obj.Param().GetButtonSe(), 10);
  EXPECT_EQ(default_obj.Param().GetButtonGroup(), 2);
  EXPECT_EQ(default_obj.Param().GetButtonNumber(), 3);

  default_obj.Param().SetButtonState(1);
  EXPECT_EQ(default_obj.Param().GetButtonState(), 1);

  default_obj.Param().SetButtonOverrides(5, 10, 15);
  EXPECT_TRUE(default_obj.Param().GetButtonUsingOverides());
  EXPECT_EQ(default_obj.Param().GetButtonPatternOverride(), 5);
  EXPECT_EQ(default_obj.Param().GetButtonXOffsetOverride(), 10);
  EXPECT_EQ(default_obj.Param().GetButtonYOffsetOverride(), 15);

  default_obj.Param().ClearButtonOverrides();
  EXPECT_FALSE(default_obj.Param().GetButtonUsingOverides());
}

TEST(ParamManagerTest, SetGetBasicProperties) {
  ParameterManager manager;

  // IsVisible
  manager.Set(ObjectProperty::IsVisible, true);
  EXPECT_EQ(manager.Get<ObjectProperty::IsVisible>(), true);

  // PositionX and PositionY
  manager.Set(ObjectProperty::PositionX, 50);
  manager.Set(ObjectProperty::PositionY, 100);
  EXPECT_EQ(manager.Get<ObjectProperty::PositionX>(), 50);
  EXPECT_EQ(manager.Get<ObjectProperty::PositionY>(), 100);

  // AdjustmentOffsetsX and AdjustmentOffsetsY
  manager.Set(ObjectProperty::AdjustmentOffsetsX, (std::array<int, 8>{5, 0}));
  EXPECT_EQ(manager.Get<ObjectProperty::AdjustmentOffsetsX>()[0], 5);

  manager.Set(ObjectProperty::AdjustmentOffsetsY,
              (std::array<int, 8>{10, -10, 0}));
  EXPECT_EQ(manager.Get<ObjectProperty::AdjustmentOffsetsY>()[1], -10);

  // AdjustmentVertical
  manager.Set(ObjectProperty::AdjustmentVertical, 1);
  EXPECT_EQ(manager.Get<ObjectProperty::AdjustmentVertical>(), 1);

  // OriginX and OriginY
  manager.Set(ObjectProperty::OriginX, 25);
  manager.Set(ObjectProperty::OriginY, 30);
  EXPECT_EQ(manager.Get<ObjectProperty::OriginX>(), 25);
  EXPECT_EQ(manager.Get<ObjectProperty::OriginY>(), 30);

  // RepetitionOriginX and RepetitionOriginY
  manager.Set(ObjectProperty::RepetitionOriginX, 15);
  manager.Set(ObjectProperty::RepetitionOriginY, 20);
  EXPECT_EQ(manager.Get<ObjectProperty::RepetitionOriginX>(), 15);
  EXPECT_EQ(manager.Get<ObjectProperty::RepetitionOriginY>(), 20);

  // WidthPercent and HeightPercent
  manager.Set(ObjectProperty::WidthPercent, 80);
  manager.Set(ObjectProperty::HeightPercent, 90);
  EXPECT_EQ(manager.Get<ObjectProperty::WidthPercent>(), 80);
  EXPECT_EQ(manager.Get<ObjectProperty::HeightPercent>(), 90);

  // HighQualityWidthPercent and HighQualityHeightPercent
  manager.Set(ObjectProperty::HighQualityWidthPercent, 800);
  manager.Set(ObjectProperty::HighQualityHeightPercent, 900);
  EXPECT_EQ(manager.Get<ObjectProperty::HighQualityWidthPercent>(), 800);
  EXPECT_EQ(manager.Get<ObjectProperty::HighQualityHeightPercent>(), 900);

  // RotationDiv10
  manager.Set(ObjectProperty::RotationDiv10, 45);
  EXPECT_EQ(manager.Get<ObjectProperty::RotationDiv10>(), 45);

  // PatternNumber
  manager.Set(ObjectProperty::PatternNumber, 5);
  EXPECT_EQ(manager.Get<ObjectProperty::PatternNumber>(), 5);

  // MonochromeTransform and InvertTransform
  manager.Set(ObjectProperty::MonochromeTransform, 1);
  EXPECT_EQ(manager.Get<ObjectProperty::MonochromeTransform>(), 1);

  manager.Set(ObjectProperty::InvertTransform, 1);
  EXPECT_EQ(manager.Get<ObjectProperty::InvertTransform>(), 1);

  // LightLevel
  manager.Set(ObjectProperty::LightLevel, 1);
  EXPECT_EQ(manager.Get<ObjectProperty::LightLevel>(), 1);

  // TintColour
  RGBColour tint(100, 150, 200);
  manager.Set(ObjectProperty::TintColour, tint);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>(), tint);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().r(), 100);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().g(), 150);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().b(), 200);

  // Modify TintColour components
  tint.set_red(110);
  manager.Set(ObjectProperty::TintColour, tint);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().r(), 110);

  tint.set_green(160);
  manager.Set(ObjectProperty::TintColour, tint);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().g(), 160);

  tint.set_blue(210);
  manager.Set(ObjectProperty::TintColour, tint);
  EXPECT_EQ(manager.Get<ObjectProperty::TintColour>().b(), 210);

  // BlendColour
  RGBAColour colour(50, 60, 70, 80);
  manager.Set(ObjectProperty::BlendColour, colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>(), colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().r(), 50);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().g(), 60);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().b(), 70);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().a(), 80);

  // Modify BlendColour components
  colour.set_red(55);
  manager.Set(ObjectProperty::BlendColour, colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().r(), 55);

  colour.set_green(65);
  manager.Set(ObjectProperty::BlendColour, colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().g(), 65);

  colour.set_blue(75);
  manager.Set(ObjectProperty::BlendColour, colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().b(), 75);

  colour.set_alpha(85);
  manager.Set(ObjectProperty::BlendColour, colour);
  EXPECT_EQ(manager.Get<ObjectProperty::BlendColour>().a(), 85);

  // CompositeMode
  manager.Set(ObjectProperty::CompositeMode, 2);
  EXPECT_EQ(manager.Get<ObjectProperty::CompositeMode>(), 2);

  // ScrollRateX and ScrollRateY
  manager.Set(ObjectProperty::ScrollRateX, 5);
  manager.Set(ObjectProperty::ScrollRateY, -5);
  EXPECT_EQ(manager.Get<ObjectProperty::ScrollRateX>(), 5);
  EXPECT_EQ(manager.Get<ObjectProperty::ScrollRateY>(), -5);

  // ZOrder, ZLayer, ZDepth
  manager.Set(ObjectProperty::ZOrder, 1);
  manager.Set(ObjectProperty::ZLayer, 2);
  manager.Set(ObjectProperty::ZDepth, 3);
  EXPECT_EQ(manager.Get<ObjectProperty::ZOrder>(), 1);
  EXPECT_EQ(manager.Get<ObjectProperty::ZLayer>(), 2);
  EXPECT_EQ(manager.Get<ObjectProperty::ZDepth>(), 3);

  // AlphaSource
  manager.Set(ObjectProperty::AlphaSource, 128);
  EXPECT_EQ(manager.Get<ObjectProperty::AlphaSource>(), 128);

  // AdjustmentAlphas
  manager.Set(ObjectProperty::AdjustmentAlphas,
              (std::array<int, 8>{5, -5, 20, -20, 0}));
  EXPECT_EQ(manager.Get<ObjectProperty::AdjustmentAlphas>()[3], -20);

  // ClippingRegion
  Rect clip_rect = Rect::GRP(0, 0, 100, 100);
  manager.Set(ObjectProperty::ClippingRegion, clip_rect);
  EXPECT_EQ(manager.Get<ObjectProperty::ClippingRegion>(), clip_rect);

  // Clear ClippingRegion
  manager.Set(ObjectProperty::ClippingRegion, Rect());
  EXPECT_TRUE(manager.Get<ObjectProperty::ClippingRegion>().is_empty());

  // OwnSpaceClippingRegion
  Rect own_clip_rect = Rect::GRP(10, 10, 80, 80);
  manager.Set(ObjectProperty::OwnSpaceClippingRegion, own_clip_rect);
  EXPECT_EQ(manager.Get<ObjectProperty::OwnSpaceClippingRegion>(),
            own_clip_rect);

  // Clear OwnSpaceClippingRegion
  manager.Set(ObjectProperty::OwnSpaceClippingRegion, Rect());
  EXPECT_TRUE(manager.Get<ObjectProperty::OwnSpaceClippingRegion>().is_empty());

  // WipeCopy
  manager.Set(ObjectProperty::WipeCopy, 1);
  EXPECT_EQ(manager.Get<ObjectProperty::WipeCopy>(), 1);
}
