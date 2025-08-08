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

#include "object/parameter_manager.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

TEST(ParameterManagerTest, DefaultInit) {
  ParameterManager default_param;

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
  EXPECT_EQ(default_param.ScaleX(), 100);
  EXPECT_EQ(default_param.ScaleY(), 100);
  EXPECT_EQ(default_param.HqScaleX(), 1000);
  EXPECT_EQ(default_param.HqScaleY(), 1000);
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
  EXPECT_EQ(default_param.Get<ObjectProperty::AdjustmentAlphas>(),
            (std::array<int, 8>{255, 255, 255, 255, 255, 255, 255, 255}));
  EXPECT_FALSE(default_param.has_clip_rect());
  EXPECT_EQ(default_param.wipe_copy(), 0);
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

  // ScaleXPercent and ScaleYPercent
  manager.Set(ObjectProperty::ScaleXPercent, 80);
  manager.Set(ObjectProperty::ScaleYPercent, 90);
  EXPECT_EQ(manager.Get<ObjectProperty::ScaleXPercent>(), 80);
  EXPECT_EQ(manager.Get<ObjectProperty::ScaleYPercent>(), 90);

  // HighQualityScaleXPercent and HighQualityScaleYPercent
  manager.Set(ObjectProperty::HighQualityScaleXPercent, 800);
  manager.Set(ObjectProperty::HighQualityScaleYPercent, 900);
  EXPECT_EQ(manager.Get<ObjectProperty::HighQualityScaleXPercent>(), 800);
  EXPECT_EQ(manager.Get<ObjectProperty::HighQualityScaleYPercent>(), 900);

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

TEST(ParameterManagerTest, TextProperties) {
  ParameterManager manager;
  manager.SetTextText("Hello World");
  EXPECT_EQ(manager.GetTextText(), "Hello World");
  manager.SetTextOps(12, 2, 3, 5, 255, 128);
  EXPECT_EQ(manager.GetTextSize(), 12);
  EXPECT_EQ(manager.GetTextXSpace(), 2);
  EXPECT_EQ(manager.GetTextYSpace(), 3);
  EXPECT_EQ(manager.GetTextCharCount(), 5);
  EXPECT_EQ(manager.GetTextColour(), 255);
  EXPECT_EQ(manager.GetTextShadowColour(), 128);

  auto text_props = manager.Get<ObjectProperty::TextProperties>();
  EXPECT_EQ(text_props.ToString(),
            "value=\"Hello World\", text_size=12, xspace=2, yspace=3, "
            "char_count=5, colour=255, shadow_colour=128");
}

TEST(ParameterManagerTest, DriftProperties) {
  ParameterManager manager;

  Rect drift_area = Rect::GRP(0, 0, 100, 100);
  manager.SetDriftOpts(10, 1, 0, 5, 1000, 2, 50, 10, 1, 0, 1, drift_area);

  auto drift_props = manager.Get<ObjectProperty::DriftProperties>();
  EXPECT_EQ(drift_props.ToString(),
            "count=10, use_animation=1, start_pattern=0, end_pattern=5, "
            "total_animation_time_ms=1000, yspeed=2, period=50, amplitude=10, "
            "use_drift=1, unknown_drift_property=0, driftspeed=1, "
            "drift_area={Rect(0, 0, Size(100, 100))}");
}

TEST(ParameterManagerTest, DigitProperties) {
  ParameterManager manager;
  manager.SetDigitValue(12345);
  manager.SetDigitOpts(5, 1, 1, 0, 2);

  auto digit_props = manager.Get<ObjectProperty::DigitProperties>();
  EXPECT_EQ(digit_props.ToString(),
            "value=12345, digits=5, zero=1, sign=1, pack=0, space=2");
}

TEST(ParameterManagerTest, ButtonProperties) {
  ParameterManager manager;
  manager.SetButtonOpts(1, 10, 2, 3);
  manager.SetButtonState(1);
  manager.SetButtonOverrides(5, 10, 15);

  auto btn_props = manager.Get<ObjectProperty::ButtonProperties>();
  EXPECT_EQ(btn_props.ToString(),
            "is_button=1, action=1, se=10, group=2, button_number=3, state=1, "
            "using_overides=true, pattern_override=5, x_offset_override=10, "
            "y_offset_override=15");

  manager.ClearButtonOverrides();
  EXPECT_FALSE(manager.Get<ObjectProperty::ButtonProperties>().using_overides);
}

TEST(ParameterManagerTest, GetterProxy) {
  ParameterManager manager;
  auto repnoGetter = CreateGetter<ObjectProperty::AdjustmentOffsetsX>();
  auto intGetter = CreateGetter<ObjectProperty::AlphaSource>();

  manager.Set(ObjectProperty::AdjustmentOffsetsX,
              std::array<int, 8>{1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(repnoGetter(manager, 1), 2);
  manager.Set(ObjectProperty::AdjustmentOffsetsX,
              std::array<int, 8>{10, -10, 20, -20});
  EXPECT_EQ(repnoGetter(manager, 2), 20);

  manager.Set(ObjectProperty::AlphaSource, 128);
  EXPECT_EQ(intGetter(manager), 128);
  manager.Set(ObjectProperty::AlphaSource, 255);
  EXPECT_EQ(intGetter(manager), 255);
}

TEST(ParameterManagerTest, SetterProxy) {
  ParameterManager manager;

  auto intSetter = CreateSetter<ObjectProperty::CompositeMode>();
  intSetter(manager, 12);
  EXPECT_EQ(manager.Get<ObjectProperty::CompositeMode>(), 12);
  intSetter(manager, 36);
  EXPECT_EQ(manager.Get<ObjectProperty::CompositeMode>(), 36);

  auto repnoSetter = CreateSetter<ObjectProperty::AdjustmentOffsetsY>();
  repnoSetter(manager, 2, 24);
  repnoSetter(manager, 3, -12);
  EXPECT_EQ(manager.Get<ObjectProperty::AdjustmentOffsetsY>(),
            (std::array<int, 8>{0, 0, 24, -12}));
}

TEST(ParameterManagerTest, Serialization) {
  std::stringstream ss;

  {
    ParameterManager manager;
    manager.Set(ObjectProperty::IsVisible, true);
    manager.Set(ObjectProperty::PositionX, 50);
    manager.Set(ObjectProperty::PositionY, 100);
    manager.Set(ObjectProperty::AdjustmentOffsetsX, (std::array<int, 8>{5, 0}));
    manager.Set(ObjectProperty::AdjustmentOffsetsY,
                (std::array<int, 8>{10, -10, 0}));
    manager.Set(ObjectProperty::BlendColour, RGBAColour(1, 2, 3, 4));
    manager.Set(ObjectProperty::TintColour, RGBColour(5, 6, 7));

    auto textp = TextProperties{"This is a sample text.", 1, 2, 3, 4, 5, 6};
    manager.Set(ObjectProperty::TextProperties, textp);

    DriftProperties driftp{1, 2, 3, 4,  5,  6,
                           7, 8, 9, 10, 11, Rect::GRP(12, 13, 14, 15)};
    manager.Set(ObjectProperty::DriftProperties, driftp);

    DigitProperties digitp{16, 17, 18, 19, 20, 21};
    manager.Set(ObjectProperty::DigitProperties, digitp);

    ButtonProperties buttonp{1, 22, 23, 24, 25, 26, true, 27, 28, 29};
    manager.Set(ObjectProperty::ButtonProperties, buttonp);

    boost::archive::text_oarchive oa(ss);
    oa << manager;
  }

  {
    boost::archive::text_iarchive ia(ss);
    ParameterManager deserialized;

    ia >> deserialized;
    EXPECT_TRUE(deserialized.Get<ObjectProperty::IsVisible>());
    EXPECT_EQ(deserialized.Get<ObjectProperty::PositionX>(), 50);
    EXPECT_EQ(deserialized.Get<ObjectProperty::PositionY>(), 100);
    EXPECT_EQ(deserialized.Get<ObjectProperty::AdjustmentOffsetsX>()[0], 5);
    EXPECT_EQ(deserialized.Get<ObjectProperty::AdjustmentOffsetsY>()[1], -10);
    EXPECT_EQ(deserialized.Get<ObjectProperty::BlendColour>(),
              RGBAColour(1, 2, 3, 4));
    EXPECT_EQ(deserialized.Get<ObjectProperty::TintColour>(),
              RGBColour(5, 6, 7));
    EXPECT_EQ(deserialized.Get<ObjectProperty::TextProperties>().ToString(),
              "value=\"This is a sample text.\", text_size=1, xspace=2, "
              "yspace=3, char_count=4, colour=5, shadow_colour=6");
    EXPECT_EQ(deserialized.Get<ObjectProperty::DriftProperties>().ToString(),
              "count=1, use_animation=2, start_pattern=3, end_pattern=4, "
              "total_animation_time_ms=5, yspeed=6, period=7, amplitude=8, "
              "use_drift=9, unknown_drift_property=10, driftspeed=11, "
              "drift_area={Rect(12, 13, Size(2, 2))}");
    EXPECT_EQ(deserialized.Get<ObjectProperty::DigitProperties>().ToString(),
              "value=16, digits=17, zero=18, sign=19, pack=20, space=21");
    EXPECT_EQ(deserialized.Get<ObjectProperty::ButtonProperties>().ToString(),
              "is_button=1, action=22, se=23, group=24, button_number=25, "
              "state=26, using_overides=true, pattern_override=27, "
              "x_offset_override=28, y_offset_override=29");
  }
}
