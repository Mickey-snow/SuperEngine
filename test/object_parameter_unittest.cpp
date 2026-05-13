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

#include "core/object_internal/object_parameter.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <sstream>

TEST(ObjectParameterTest, DefaultInit) {
  ObjectParameter param;

  EXPECT_FALSE(param.is_visible);
  EXPECT_EQ(param.position_x, 0);
  EXPECT_EQ(param.position_y, 0);
  EXPECT_EQ(param.GetXAdjustmentSum(), 0);
  EXPECT_EQ(param.GetYAdjustmentSum(), 0);
  EXPECT_EQ(param.adjustment_vertical, 0);
  EXPECT_EQ(param.origin_x, 0);
  EXPECT_EQ(param.origin_y, 0);
  EXPECT_EQ(param.repetition_origin_x, 0);
  EXPECT_EQ(param.repetition_origin_y, 0);
  EXPECT_EQ(param.scale_x_percent, 100);
  EXPECT_EQ(param.scale_y_percent, 100);
  EXPECT_EQ(param.high_quality_scale_x_percent, 1000);
  EXPECT_EQ(param.high_quality_scale_y_percent, 1000);
  EXPECT_EQ(param.rotation_div10, 0);
  EXPECT_EQ(param.pattern_number, 0);
  EXPECT_EQ(param.alpha_source, 255);
  EXPECT_EQ(param.adjustment_alphas,
            (std::array<int, 8>{255, 255, 255, 255, 255, 255, 255, 255}));
  EXPECT_FALSE(param.has_clip_rect());
  EXPECT_FALSE(param.has_own_clip_rect());
  EXPECT_EQ(param.monochrome_transform, 0);
  EXPECT_EQ(param.invert_transform, 0);
  EXPECT_EQ(param.light_level, 0);
  EXPECT_EQ(param.tint_colour, RGBColour(0, 0, 0));
  EXPECT_EQ(param.blend_colour, RGBAColour(0, 0, 0, 0));
  EXPECT_EQ(param.composite_mode, 0);
  EXPECT_EQ(param.scroll_rate_x, 0);
  EXPECT_EQ(param.scroll_rate_y, 0);
  EXPECT_EQ(param.z_order, 0);
  EXPECT_EQ(param.z_layer, 0);
  EXPECT_EQ(param.z_depth, 0);
  EXPECT_EQ(param.wipe_copy, 0);
}

TEST(ObjectParameterTest, AccessorsUpdateTypedFields) {
  ObjectParameter param;

  param.SetVisible(1);
  param.SetX(50);
  param.SetY(100);
  param.SetXAdjustment(0, 5);
  param.SetYAdjustment(1, -10);
  param.SetVert(1);
  param.SetOriginX(25);
  param.SetOriginY(30);
  param.SetRepOriginX(15);
  param.SetRepOriginY(20);
  param.SetScaleX(80);
  param.SetScaleY(90);
  param.SetHqScaleX(800);
  param.SetHqScaleY(900);
  param.SetRotation(45);
  param.SetPattNo(5);
  param.SetMono(1);
  param.SetInvert(1);
  param.SetLight(1);
  param.SetTint(RGBColour(100, 150, 200));
  param.SetTintRed(110);
  param.SetTintGreen(160);
  param.SetTintBlue(210);
  param.SetColour(RGBAColour(50, 60, 70, 80));
  param.SetColourRed(55);
  param.SetColourGreen(65);
  param.SetColourBlue(75);
  param.SetColourLevel(85);
  param.SetCompositeMode(2);
  param.SetScrollRateX(5);
  param.SetScrollRateY(-5);
  param.SetZOrder(1);
  param.SetZLayer(2);
  param.SetZDepth(3);
  param.SetAlpha(128);
  param.SetAlphaAdjustment(3, -20);
  param.SetClipRect(Rect::GRP(0, 0, 100, 100));
  param.SetOwnClipRect(Rect::GRP(10, 10, 80, 80));
  param.SetWipeCopy(1);

  EXPECT_EQ(param.visible(), 1);
  EXPECT_EQ(param.x(), 50);
  EXPECT_EQ(param.y(), 100);
  EXPECT_EQ(param.x_adjustment(0), 5);
  EXPECT_EQ(param.y_adjustment(1), -10);
  EXPECT_EQ(param.vert(), 1);
  EXPECT_EQ(param.origin_x, 25);
  EXPECT_EQ(param.origin_y, 30);
  EXPECT_EQ(param.rep_origin_x(), 15);
  EXPECT_EQ(param.rep_origin_y(), 20);
  EXPECT_EQ(param.ScaleX(), 80);
  EXPECT_EQ(param.ScaleY(), 90);
  EXPECT_EQ(param.HqScaleX(), 800);
  EXPECT_EQ(param.HqScaleY(), 900);
  EXPECT_EQ(param.rotation(), 45);
  EXPECT_EQ(param.GetPattNo(), 5);
  EXPECT_EQ(param.mono(), 1);
  EXPECT_EQ(param.invert(), 1);
  EXPECT_EQ(param.light(), 1);
  EXPECT_EQ(param.tint(), RGBColour(110, 160, 210));
  EXPECT_EQ(param.colour(), RGBAColour(55, 65, 75, 85));
  EXPECT_EQ(param.composite_mode, 2);
  EXPECT_EQ(param.scroll_rate_x, 5);
  EXPECT_EQ(param.scroll_rate_y, -5);
  EXPECT_EQ(param.z_order, 1);
  EXPECT_EQ(param.z_layer, 2);
  EXPECT_EQ(param.z_depth, 3);
  EXPECT_EQ(param.raw_alpha(), 128);
  EXPECT_EQ(param.alpha_adjustment(3), -20);
  EXPECT_TRUE(param.has_clip_rect());
  EXPECT_TRUE(param.has_own_clip_rect());
  EXPECT_EQ(param.wipe_copy, 1);
}

TEST(ObjectParameterTest, CompoundProperties) {
  ObjectParameter param;

  param.SetTextText("Hello World");
  param.SetTextOps(12, 2, 3, 5, 255, 128);
  EXPECT_EQ(param.text.ToString(),
            "value=\"Hello World\", text_size=12, xspace=2, yspace=3, "
            "char_count=5, colour=255, shadow_colour=128");

  Rect drift_area = Rect::GRP(0, 0, 100, 100);
  param.SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1, drift_area);
  EXPECT_EQ(param.drift.ToString(),
            "count=10, use_animation=1, start_pattern=0, end_pattern=5, "
            "total_animation_time_ms=1000, yspeed=2, period=50, amplitude=10, "
            "use_drift=1, unknown_drift_property=0, driftspeed=1, "
            "drift_area={Rect(0, 0, Size(100, 100))}");

  param.SetDigitValue(12345);
  param.SetDigitOpts(5, 1, 1, 0, 2);
  EXPECT_EQ(param.digit.ToString(),
            "value=12345, digits=5, zero=1, sign=1, pack=0, space=2");

  param.SetButtonOpts(1, 10, 2, 3);
  param.SetButtonState(1);
  param.SetButtonOverrides(5, 10, 15);
  EXPECT_EQ(param.button.ToString(),
            "is_button=1, action=1, se=10, group=2, button_number=3, state=1, "
            "using_overides=true, pattern_override=5, x_offset_override=10, "
            "y_offset_override=15");

  param.ClearButtonOverrides();
  EXPECT_FALSE(param.button.using_overides);
}

TEST(ObjectParameterTest, GetterProxy) {
  ObjectParameter param;
  auto repno_getter = CreateGetter<&ObjectParameter::adjustment_offsets_x>();
  auto int_getter = CreateGetter<&ObjectParameter::alpha_source>();

  param.adjustment_offsets_x = {1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_EQ(repno_getter(param, 1), 2);
  param.adjustment_offsets_x = {10, -10, 20, -20};
  EXPECT_EQ(repno_getter(param, 2), 20);

  param.alpha_source = 128;
  EXPECT_EQ(int_getter(param), 128);
  param.alpha_source = 255;
  EXPECT_EQ(int_getter(param), 255);
}

TEST(ObjectParameterTest, SetterProxy) {
  ObjectParameter param;

  auto int_setter = CreateSetter<&ObjectParameter::composite_mode>();
  int_setter(param, 12);
  EXPECT_EQ(param.composite_mode, 12);
  int_setter(param, 36);
  EXPECT_EQ(param.composite_mode, 36);

  auto repno_setter = CreateSetter<&ObjectParameter::adjustment_offsets_y>();
  repno_setter(param, 2, 24);
  repno_setter(param, 3, -12);
  EXPECT_EQ(param.adjustment_offsets_y, (std::array<int, 8>{0, 0, 24, -12}));
}

TEST(ObjectParameterTest, Serialization) {
  std::stringstream ss;

  {
    ObjectParameter param;
    param.is_visible = true;
    param.position_x = 50;
    param.position_y = 100;
    param.adjustment_offsets_x = {5, 0};
    param.adjustment_offsets_y = {10, -10, 0};
    param.blend_colour = RGBAColour(1, 2, 3, 4);
    param.tint_colour = RGBColour(5, 6, 7);
    param.text = TextProperties{"This is a sample text.", 1, 2, 3, 4, 5, 6};
    param.drift =
        DriftProperties{1, 2, 3, 4,  5,  6,
                        7, 8, 9, 10, 11, Rect::GRP(12, 13, 14, 15)};
    param.digit = DigitProperties{16, 17, 18, 19, 20, 21};
    param.button = ButtonProperties{1, 22, 23, 24, 25,
                                    26, true, 27, 28, 29};

    boost::archive::text_oarchive oa(ss);
    oa << param;
  }

  {
    boost::archive::text_iarchive ia(ss);
    ObjectParameter deserialized;

    ia >> deserialized;
    EXPECT_TRUE(deserialized.is_visible);
    EXPECT_EQ(deserialized.position_x, 50);
    EXPECT_EQ(deserialized.position_y, 100);
    EXPECT_EQ(deserialized.adjustment_offsets_x[0], 5);
    EXPECT_EQ(deserialized.adjustment_offsets_y[1], -10);
    EXPECT_EQ(deserialized.blend_colour, RGBAColour(1, 2, 3, 4));
    EXPECT_EQ(deserialized.tint_colour, RGBColour(5, 6, 7));
    EXPECT_EQ(deserialized.text.ToString(),
              "value=\"This is a sample text.\", text_size=1, xspace=2, "
              "yspace=3, char_count=4, colour=5, shadow_colour=6");
    EXPECT_EQ(deserialized.drift.ToString(),
              "count=1, use_animation=2, start_pattern=3, end_pattern=4, "
              "total_animation_time_ms=5, yspeed=6, period=7, amplitude=8, "
              "use_drift=9, unknown_drift_property=10, driftspeed=11, "
              "drift_area={Rect(12, 13, Size(2, 2))}");
    EXPECT_EQ(deserialized.digit.ToString(),
              "value=16, digits=17, zero=18, sign=19, pack=20, space=21");
    EXPECT_EQ(deserialized.button.ToString(),
              "is_button=1, action=22, se=23, group=24, button_number=25, "
              "state=26, using_overides=true, pattern_override=27, "
              "x_offset_override=28, y_offset_override=29");
  }
}
