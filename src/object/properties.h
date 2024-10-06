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

[[maybe_unused]] constexpr int DEFAULT_TEXT_SIZE = 14;
[[maybe_unused]] constexpr int DEFAULT_TEXT_XSPACE = 0;
[[maybe_unused]] constexpr int DEFAULT_TEXT_YSPACE = 0;
[[maybe_unused]] constexpr int DEFAULT_TEXT_CHAR_COUNT = 0;
[[maybe_unused]] constexpr int DEFAULT_TEXT_COLOUR = 0;
[[maybe_unused]] constexpr int DEFAULT_TEXT_SHADOWCOLOUR = -1;

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

[[maybe_unused]] constexpr int DEFAULT_DRIFT_COUNT = 1;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_USE_ANIMATION = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_START_PATTERN = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_END_PATTERN = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_ANIMATION_TIME = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_YSPEED = 1000;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_PERIOD = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_AMPLITUDE = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_USE_DRIFT = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_UNKNOWN_PROP = 0;
[[maybe_unused]] constexpr int DEFAULT_DRIFT_DRIFTSPEED = 0;
[[maybe_unused]] const Rect DEFAULT_DRIFT_AREA =
    Rect(Point(-1, -1), Size(-1, -1));

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

[[maybe_unused]] constexpr int DEFAULT_DIGITS_VALUE = 0;
[[maybe_unused]] constexpr int DEFAULT_DIGITS_DIGITS = 0;
[[maybe_unused]] constexpr int DEFAULT_DIGITS_ZERO = 0;
[[maybe_unused]] constexpr int DEFAULT_DIGITS_SIGN = 0;
[[maybe_unused]] constexpr int DEFAULT_DIGITS_PACK = 0;
[[maybe_unused]] constexpr int DEFAULT_DIGITS_SPACE = 0;

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

[[maybe_unused]] constexpr int DEFAULT_BUTTON_IS_BUTTON = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_ACTION = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_SE = -1;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_GROUP = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_NUMBER = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_STATE = 0;
[[maybe_unused]] constexpr bool DEFAULT_BUTTON_USING_OVERRIDES = false;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_PATTERN_OVERRIDE = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_X_OFFSET = 0;
[[maybe_unused]] constexpr int DEFAULT_BUTTON_Y_OFFSET = 0;

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
