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

#include "object/properties.h"

#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"

// -----------------------------------------------------------------------
// TextProperties
// -----------------------------------------------------------------------
TextProperties::TextProperties()
    : value(),
      text_size(DEFAULT_TEXT_SIZE),
      xspace(DEFAULT_TEXT_XSPACE),
      yspace(DEFAULT_TEXT_YSPACE),
      char_count(DEFAULT_TEXT_CHAR_COUNT),
      colour(DEFAULT_TEXT_COLOUR),
      shadow_colour(DEFAULT_TEXT_SHADOWCOLOUR) {}

template <class Archive>
void TextProperties::serialize(Archive& ar, unsigned int version) {
  ar & value & text_size & xspace & yspace & char_count & colour &
      shadow_colour;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void TextProperties::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void TextProperties::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);

// -----------------------------------------------------------------------
// DirftProperties
// -----------------------------------------------------------------------

DriftProperties::DriftProperties()
    : count(DEFAULT_DRIFT_COUNT),
      use_animation(DEFAULT_DRIFT_USE_ANIMATION),
      start_pattern(DEFAULT_DRIFT_START_PATTERN),
      end_pattern(DEFAULT_DRIFT_END_PATTERN),
      total_animation_time_ms(DEFAULT_DRIFT_ANIMATION_TIME),
      yspeed(DEFAULT_DRIFT_YSPEED),
      period(DEFAULT_DRIFT_PERIOD),
      amplitude(DEFAULT_DRIFT_AMPLITUDE),
      use_drift(DEFAULT_DRIFT_USE_DRIFT),
      unknown_drift_property(DEFAULT_DRIFT_UNKNOWN_PROP),
      driftspeed(DEFAULT_DRIFT_DRIFTSPEED),
      drift_area(DEFAULT_DRIFT_AREA) {}

template <class Archive>
void DriftProperties::serialize(Archive& ar, unsigned int version) {
  ar & count & use_animation & start_pattern & end_pattern &
      total_animation_time_ms & yspeed & period & amplitude & use_drift &
      unknown_drift_property & driftspeed & drift_area;
}

// -----------------------------------------------------------------------

template void DriftProperties::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void DriftProperties::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);

// -----------------------------------------------------------------------
// DigitProperties
// -----------------------------------------------------------------------

DigitProperties::DigitProperties()
    : value(DEFAULT_DIGITS_VALUE),
      digits(DEFAULT_DIGITS_DIGITS),
      zero(DEFAULT_DIGITS_ZERO),
      sign(DEFAULT_DIGITS_SIGN),
      pack(DEFAULT_DIGITS_PACK),
      space(DEFAULT_DIGITS_SPACE) {}

template <class Archive>
void DigitProperties::serialize(Archive& ar, unsigned int version) {
  ar & value & digits & zero & sign & pack & space;
}

// -----------------------------------------------------------------------

template void DigitProperties::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void DigitProperties::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);

// -----------------------------------------------------------------------
// ButtonProperties
// -----------------------------------------------------------------------

ButtonProperties::ButtonProperties()
    : is_button(DEFAULT_BUTTON_IS_BUTTON),
      action(DEFAULT_BUTTON_ACTION),
      se(DEFAULT_BUTTON_SE),
      group(DEFAULT_BUTTON_GROUP),
      button_number(DEFAULT_BUTTON_NUMBER),
      state(DEFAULT_BUTTON_STATE),
      using_overides(DEFAULT_BUTTON_USING_OVERRIDES),
      pattern_override(DEFAULT_BUTTON_PATTERN_OVERRIDE),
      x_offset_override(DEFAULT_BUTTON_X_OFFSET),
      y_offset_override(DEFAULT_BUTTON_Y_OFFSET) {}

template <class Archive>
void ButtonProperties::serialize(Archive& ar, unsigned int version) {
  // The override values are stuck here because I'm not sure about
  // initialization otherwise.
  ar & is_button & action & se & group & button_number & state &
      using_overides & pattern_override & x_offset_override & y_offset_override;
}

// -----------------------------------------------------------------------

template void ButtonProperties::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void ButtonProperties::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
