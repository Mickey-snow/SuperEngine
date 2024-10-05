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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

class GraphicsObjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // uncategorized basic properties
    obj.SetVert(1);
    obj.SetOriginX(25);
    obj.SetOriginY(30);
    obj.SetRepOriginX(15);
    obj.SetRepOriginY(20);
    obj.SetWidth(80);
    obj.SetHeight(90);
    obj.SetHqWidth(800);
    obj.SetHqHeight(900);
    obj.SetRotation(45);
    obj.SetPattNo(5);
    obj.SetMono(1);
    obj.SetInvert(1);
    obj.SetLight(1);
    RGBColour tint(100, 150, 200);
    obj.SetTint(tint);
    RGBAColour colour(50, 60, 70, 80);
    obj.SetColour(colour);
    Rect clip_rect = Rect::GRP(0, 0, 100, 100);
    obj.SetClipRect(clip_rect);

    // text properties
    obj.SetTextText("Hello World");
    obj.SetTextOps(12, 2, 3, 5, 255, 128);

    // drift properties
    Rect drift_area = Rect::GRP(0, 0, 100, 100);
    obj.SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1, drift_area);

    // digit properties
    obj.SetDigitValue(12345);
    obj.SetDigitOpts(5, 1, 1, 0, 2);

    // button properties
    obj.SetButtonOpts(1, 10, 2, 3);
    obj.SetButtonState(1);
    obj.SetButtonOverrides(5, 10, 15);
  }

  GraphicsObject obj, default_obj;
};

TEST_F(GraphicsObjectTest, DefaultBasicProperty) {
  EXPECT_EQ(default_obj.visible(), false);
  EXPECT_EQ(default_obj.x(), 0);
  EXPECT_EQ(default_obj.y(), 0);
  EXPECT_EQ(default_obj.GetXAdjustmentSum(), 0);
  EXPECT_EQ(default_obj.GetYAdjustmentSum(), 0);
  EXPECT_EQ(default_obj.vert(), 0);
  EXPECT_EQ(default_obj.origin_x(), 0);
  EXPECT_EQ(default_obj.origin_y(), 0);
  EXPECT_EQ(default_obj.rep_origin_x(), 0);
  EXPECT_EQ(default_obj.rep_origin_y(), 0);
  EXPECT_EQ(default_obj.width(), 100);
  EXPECT_EQ(default_obj.height(), 100);
  EXPECT_EQ(default_obj.hq_width(), 1000);
  EXPECT_EQ(default_obj.hq_height(), 1000);
  EXPECT_EQ(default_obj.rotation(), 0);
  EXPECT_EQ(default_obj.PixelWidth(), 0);
  EXPECT_EQ(default_obj.PixelHeight(), 0);
  EXPECT_EQ(default_obj.GetPattNo(), 0);
  EXPECT_EQ(default_obj.mono(), 0);
  EXPECT_EQ(default_obj.invert(), 0);
  EXPECT_EQ(default_obj.light(), 0);
  EXPECT_EQ(default_obj.tint(), RGBColour(0, 0, 0));
  EXPECT_EQ(default_obj.colour(), RGBAColour(0, 0, 0, 0));
  EXPECT_EQ(default_obj.composite_mode(), 0);
  EXPECT_EQ(default_obj.scroll_rate_x(), 0);
  EXPECT_EQ(default_obj.scroll_rate_y(), 0);
  EXPECT_EQ(default_obj.z_order(), 0);
  EXPECT_EQ(default_obj.z_layer(), 0);
  EXPECT_EQ(default_obj.z_depth(), 0);
  EXPECT_EQ(default_obj.raw_alpha(), 255);
  EXPECT_FALSE(default_obj.has_object_data());
  EXPECT_FALSE(default_obj.has_clip_rect());
  EXPECT_EQ(default_obj.wipe_copy(), 0);
}

TEST_F(GraphicsObjectTest, SetGetBasicProperties) {
  default_obj.SetVisible(true);
  EXPECT_EQ(default_obj.visible(), true);

  default_obj.SetX(50);
  EXPECT_EQ(default_obj.x(), 50);

  default_obj.SetY(100);
  EXPECT_EQ(default_obj.y(), 100);

  default_obj.SetXAdjustment(0, 5);
  EXPECT_EQ(default_obj.x_adjustment(0), 5);
  EXPECT_EQ(default_obj.GetXAdjustmentSum(), 5);

  default_obj.SetYAdjustment(0, -10);
  EXPECT_EQ(default_obj.y_adjustment(0), -10);
  EXPECT_EQ(default_obj.GetYAdjustmentSum(), -10);

  default_obj.SetVert(1);
  EXPECT_EQ(default_obj.vert(), 1);

  default_obj.SetOriginX(25);
  EXPECT_EQ(default_obj.origin_x(), 25);

  default_obj.SetOriginY(30);
  EXPECT_EQ(default_obj.origin_y(), 30);

  default_obj.SetRepOriginX(15);
  EXPECT_EQ(default_obj.rep_origin_x(), 15);

  default_obj.SetRepOriginY(20);
  EXPECT_EQ(default_obj.rep_origin_y(), 20);

  default_obj.SetWidth(80);
  EXPECT_EQ(default_obj.width(), 80);

  default_obj.SetHeight(90);
  EXPECT_EQ(default_obj.height(), 90);

  default_obj.SetHqWidth(800);
  EXPECT_EQ(default_obj.hq_width(), 800);

  default_obj.SetHqHeight(900);
  EXPECT_EQ(default_obj.hq_height(), 900);

  default_obj.SetRotation(45);
  EXPECT_EQ(default_obj.rotation(), 45);

  default_obj.SetPattNo(5);
  EXPECT_EQ(default_obj.GetPattNo(), 5);

  default_obj.SetMono(1);
  EXPECT_EQ(default_obj.mono(), 1);

  default_obj.SetInvert(1);
  EXPECT_EQ(default_obj.invert(), 1);

  default_obj.SetLight(1);
  EXPECT_EQ(default_obj.light(), 1);

  RGBColour tint(100, 150, 200);
  default_obj.SetTint(tint);
  EXPECT_EQ(default_obj.tint(), tint);
  EXPECT_EQ(default_obj.tint_red(), 100);
  EXPECT_EQ(default_obj.tint_green(), 150);
  EXPECT_EQ(default_obj.tint_blue(), 200);

  default_obj.SetTintRed(110);
  EXPECT_EQ(default_obj.tint_red(), 110);

  default_obj.SetTintGreen(160);
  EXPECT_EQ(default_obj.tint_green(), 160);

  default_obj.SetTintBlue(210);
  EXPECT_EQ(default_obj.tint_blue(), 210);

  RGBAColour colour(50, 60, 70, 80);
  default_obj.SetColour(colour);
  EXPECT_EQ(default_obj.colour(), colour);
  EXPECT_EQ(default_obj.colour_red(), 50);
  EXPECT_EQ(default_obj.colour_green(), 60);
  EXPECT_EQ(default_obj.colour_blue(), 70);
  EXPECT_EQ(default_obj.colour_level(), 80);

  default_obj.SetColourRed(55);
  EXPECT_EQ(default_obj.colour_red(), 55);

  default_obj.SetColourGreen(65);
  EXPECT_EQ(default_obj.colour_green(), 65);

  default_obj.SetColourBlue(75);
  EXPECT_EQ(default_obj.colour_blue(), 75);

  default_obj.SetColourLevel(85);
  EXPECT_EQ(default_obj.colour_level(), 85);

  default_obj.SetCompositeMode(2);
  EXPECT_EQ(default_obj.composite_mode(), 2);

  default_obj.SetScrollRateX(5);
  EXPECT_EQ(default_obj.scroll_rate_x(), 5);

  default_obj.SetScrollRateY(-5);
  EXPECT_EQ(default_obj.scroll_rate_y(), -5);

  default_obj.SetZOrder(1);
  EXPECT_EQ(default_obj.z_order(), 1);

  default_obj.SetZLayer(2);
  EXPECT_EQ(default_obj.z_layer(), 2);

  default_obj.SetZDepth(3);
  EXPECT_EQ(default_obj.z_depth(), 3);

  default_obj.SetAlpha(128);
  EXPECT_EQ(default_obj.raw_alpha(), 128);

  default_obj.SetAlphaAdjustment(0, 10);
  EXPECT_EQ(default_obj.alpha_adjustment(0), 10);

  Rect clip_rect = Rect::GRP(0, 0, 100, 100);
  default_obj.SetClipRect(clip_rect);
  EXPECT_TRUE(default_obj.has_clip_rect());
  EXPECT_EQ(default_obj.clip_rect(), clip_rect);

  default_obj.ClearClipRect();
  EXPECT_FALSE(default_obj.has_clip_rect());

  Rect own_clip_rect = Rect::GRP(10, 10, 80, 80);
  default_obj.SetOwnClipRect(own_clip_rect);
  EXPECT_TRUE(default_obj.has_own_clip_rect());
  EXPECT_EQ(default_obj.own_clip_rect(), own_clip_rect);

  default_obj.ClearOwnClipRect();
  EXPECT_FALSE(default_obj.has_own_clip_rect());

  default_obj.SetWipeCopy(1);
  EXPECT_EQ(default_obj.wipe_copy(), 1);
}

TEST_F(GraphicsObjectTest, TextProperties) {
  default_obj.SetTextText("Hello World");
  EXPECT_EQ(default_obj.GetTextText(), "Hello World");

  default_obj.SetTextOps(12, 2, 3, 5, 255, 128);
  EXPECT_EQ(default_obj.GetTextSize(), 12);
  EXPECT_EQ(default_obj.GetTextXSpace(), 2);
  EXPECT_EQ(default_obj.GetTextYSpace(), 3);
  EXPECT_EQ(default_obj.GetTextCharCount(), 5);
  EXPECT_EQ(default_obj.GetTextColour(), 255);
  EXPECT_EQ(default_obj.GetTextShadowColour(), 128);
}

TEST_F(GraphicsObjectTest, DriftProperties) {
  Rect drift_area = Rect::GRP(0, 0, 100, 100);
  default_obj.SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1, drift_area);

  EXPECT_EQ(default_obj.GetDriftParticleCount(), 10);
  EXPECT_EQ(default_obj.GetDriftUseAnimation(), 1);
  EXPECT_EQ(default_obj.GetDriftStartPattern(), 0);
  EXPECT_EQ(default_obj.GetDriftEndPattern(), 5);
  EXPECT_EQ(default_obj.GetDriftAnimationTime(), 1000);
  EXPECT_EQ(default_obj.GetDriftYSpeed(), 2);
  EXPECT_EQ(default_obj.GetDriftPeriod(), 50);
  EXPECT_EQ(default_obj.GetDriftAmplitude(), 10);
  EXPECT_EQ(default_obj.GetDriftUseDrift(), 1);
  EXPECT_EQ(default_obj.GetDriftUnknown(), 0);
  EXPECT_EQ(default_obj.GetDriftDriftSpeed(), 1);
  EXPECT_EQ(default_obj.GetDriftArea(), drift_area);
}

TEST_F(GraphicsObjectTest, DigitProperties) {
  default_obj.SetDigitValue(12345);
  EXPECT_EQ(default_obj.GetDigitValue(), 12345);

  default_obj.SetDigitOpts(5, 1, 1, 0, 2);
  EXPECT_EQ(default_obj.GetDigitDigits(), 5);
  EXPECT_EQ(default_obj.GetDigitZero(), 1);
  EXPECT_EQ(default_obj.GetDigitSign(), 1);
  EXPECT_EQ(default_obj.GetDigitPack(), 0);
  EXPECT_EQ(default_obj.GetDigitSpace(), 2);
}

TEST_F(GraphicsObjectTest, ButtonProperties) {
  default_obj.SetButtonOpts(1, 10, 2, 3);
  EXPECT_EQ(default_obj.IsButton(), 1);
  EXPECT_EQ(default_obj.GetButtonAction(), 1);
  EXPECT_EQ(default_obj.GetButtonSe(), 10);
  EXPECT_EQ(default_obj.GetButtonGroup(), 2);
  EXPECT_EQ(default_obj.GetButtonNumber(), 3);

  default_obj.SetButtonState(1);
  EXPECT_EQ(default_obj.GetButtonState(), 1);

  default_obj.SetButtonOverrides(5, 10, 15);
  EXPECT_TRUE(default_obj.GetButtonUsingOverides());
  EXPECT_EQ(default_obj.GetButtonPatternOverride(), 5);
  EXPECT_EQ(default_obj.GetButtonXOffsetOverride(), 10);
  EXPECT_EQ(default_obj.GetButtonYOffsetOverride(), 15);

  default_obj.ClearButtonOverrides();
  EXPECT_FALSE(default_obj.GetButtonUsingOverides());
}

TEST_F(GraphicsObjectTest, Serialize) {
  std::stringstream ss;
  GraphicsObject new_obj;

  {
    boost::archive::text_oarchive archive(ss);
    archive & obj;
  }

  {
    boost::archive::text_iarchive archive(ss);
    archive & new_obj;
  }
}
