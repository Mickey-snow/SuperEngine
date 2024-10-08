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

#include <string>

enum class ObjectProperty {
  IsVisible = 1,
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
  WipeCopy
};

struct TextProperties {
  TextProperties();

  std::string value;

  int text_size, xspace, yspace;

  int char_count;
  int colour;
  int shadow_colour;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

struct DriftProperties {
  DriftProperties();

  int count;

  int use_animation;
  int start_pattern;
  int end_pattern;
  int total_animation_time_ms;

  int yspeed;

  int period;
  int amplitude;

  int use_drift;
  int unknown_drift_property;
  int driftspeed;

  Rect drift_area;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

struct DigitProperties {
  DigitProperties();

  int value;

  int digits;
  int zero;
  int sign;
  int pack;
  int space;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

struct ButtonProperties {
  ButtonProperties();

  int is_button;

  int action;
  int se;
  int group;
  int button_number;

  int state;

  bool using_overides;
  int pattern_override;
  int x_offset_override;
  int y_offset_override;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

#endif
