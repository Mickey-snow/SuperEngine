// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#ifndef SRC_OBJECT_PROPERTIES_H_
#define SRC_OBJECT_PROPERTIES_H_

#include "base/rect.h"
#include "systems/base/colour.h"
#include "utilities/mpl.h"

#include <array>
#include <string>

#include <boost/serialization/serialization.hpp>

struct TextProperties {
  std::string value = "";

  int text_size = 14;
  int xspace = 0, yspace = 0;

  int char_count = 0;
  int colour = 0;
  int shadow_colour = -1;

  bool operator==(const TextProperties& rhs) const;
  bool operator!=(const TextProperties& rhs) const;

  // for debug
  std::string ToString() const;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & value & text_size & xspace & yspace & char_count & colour &
        shadow_colour;
  }
};

struct DriftProperties {
  int count = 1;

  int use_animation = 0;
  int start_pattern = 0;
  int end_pattern = 0;
  int total_animation_time_ms = 0;

  int yspeed = 1000;

  int period = 0;
  int amplitude = 0;

  int use_drift = 0;
  int unknown_drift_property = 0;
  int driftspeed = 0;

  Rect drift_area = Rect(Point(-1, -1), Size(-1, -1));

  bool operator==(const DriftProperties& rhs) const;
  bool operator!=(const DriftProperties& rhs) const;

  // for debug
  std::string ToString() const;

  // boost::serialization support

  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & count & use_animation & start_pattern & end_pattern &
        total_animation_time_ms & yspeed & period & amplitude & use_drift &
        unknown_drift_property & driftspeed & drift_area;
  }
};

struct DigitProperties {
  int value = 0;

  int digits = 0;
  int zero = 0;
  int sign = 0;
  int pack = 0;
  int space = 0;

  bool operator==(const DigitProperties& rhs) const;
  bool operator!=(const DigitProperties& rhs) const;

  // for debug
  std::string ToString() const;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & value & digits & zero & sign & pack & space;
  }
};

struct ButtonProperties {
  int is_button = 0;

  int action = 0;
  int se = -1;
  int group = 0;
  int button_number = 0;

  int state = 0;

  bool using_overides = false;
  int pattern_override = 0;
  int x_offset_override = 0;
  int y_offset_override = 0;

  bool operator==(const ButtonProperties& rhs) const;
  bool operator!=(const ButtonProperties& rhs) const;

  // for debug
  std::string ToString() const;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    // The override values are stuck here because I'm not sure about
    // initialization otherwise.
    ar & is_button & action & se & group & button_number & state &
        using_overides & pattern_override & x_offset_override &
        y_offset_override;
  }
};

using ObjectPropertyType = TypeList<bool,                // IsVisible
                                    int,                 // PositionX
                                    int,                 // PositionY
                                    std::array<int, 8>,  // AdjustmentOffsetsX
                                    std::array<int, 8>,  // AdjustmentOffsetsY
                                    int,                 // AdjustmentVertical
                                    int,                 // OriginX
                                    int,                 // OriginY
                                    int,                 // RepetitionOriginX
                                    int,                 // RepetitionOriginY
                                    int,                 // WidthPercent
                                    int,                 // HeightPercent
                                    int,  // HighQualityWidthPercent
                                    int,  // HighQualityHeightPercent
                                    int,  // RotationDiv10
                                    int,  // PatternNumber
                                    int,  // AlphaSource
                                    std::array<int, 8>,  // AdjustmentAlphas
                                    Rect,                // ClippingRegion
                                    Rect,              // OwnSpaceClippingRegion
                                    int,               // MonochromeTransform
                                    int,               // InvertTransform
                                    int,               // LightLevel
                                    RGBColour,         // TintColour
                                    RGBAColour,        // BlendColour
                                    int,               // CompositeMode
                                    int,               // ScrollRateX
                                    int,               // ScrollRateY
                                    int,               // ZOrder
                                    int,               // ZLayer
                                    int,               // ZDepth
                                    TextProperties,    // TextProperties
                                    DriftProperties,   // DriftProperties
                                    DigitProperties,   // DigitProperties
                                    ButtonProperties,  // ButtonProperties
                                    int                // WipeCopy
                                    >;

enum class ObjectProperty {
  IsVisible = 0,
  PositionX,
  PositionY,
  AdjustmentOffsetsX,
  AdjustmentOffsetsY,
  AdjustmentVertical,
  OriginX,
  OriginY,
  RepetitionOriginX,
  RepetitionOriginY,
  WidthPercent,
  HeightPercent,
  HighQualityWidthPercent,
  HighQualityHeightPercent,
  RotationDiv10,
  PatternNumber,
  AlphaSource,
  AdjustmentAlphas,
  ClippingRegion,
  OwnSpaceClippingRegion,
  MonochromeTransform,
  InvertTransform,
  LightLevel,
  TintColour,
  BlendColour,
  CompositeMode,
  ScrollRateX,
  ScrollRateY,
  ZOrder,
  ZLayer,
  ZDepth,
  TextProperties,
  DriftProperties,
  DigitProperties,
  ButtonProperties,
  WipeCopy,

  TOTAL_COUNT
};

#endif
