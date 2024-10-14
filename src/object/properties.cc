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

#include <sstream>

std::string TextProperties::ToString() const {
  std::ostringstream oss;
  oss << "value=\"" << value << "\"";
  oss << ", text_size=" << text_size;
  oss << ", xspace=" << xspace;
  oss << ", yspace=" << yspace;
  oss << ", char_count=" << char_count;
  oss << ", colour=" << colour;
  oss << ", shadow_colour=" << shadow_colour;
  return oss.str();
}

std::string DriftProperties::ToString() const {
  std::ostringstream oss;
  oss << "count=" << count;
  oss << ", use_animation=" << use_animation;
  oss << ", start_pattern=" << start_pattern;
  oss << ", end_pattern=" << end_pattern;
  oss << ", total_animation_time_ms=" << total_animation_time_ms;
  oss << ", yspeed=" << yspeed;
  oss << ", period=" << period;
  oss << ", amplitude=" << amplitude;
  oss << ", use_drift=" << use_drift;
  oss << ", unknown_drift_property=" << unknown_drift_property;
  oss << ", driftspeed=" << driftspeed;
  oss << ", drift_area={" << drift_area << "}";
  return oss.str();
}

std::string DigitProperties::ToString() const {
  std::ostringstream oss;
  oss << "value=" << value;
  oss << ", digits=" << digits;
  oss << ", zero=" << zero;
  oss << ", sign=" << sign;
  oss << ", pack=" << pack;
  oss << ", space=" << space;
  return oss.str();
}

std::string ButtonProperties::ToString() const {
  std::ostringstream oss;
  oss << "is_button=" << is_button;
  oss << ", action=" << action;
  oss << ", se=" << se;
  oss << ", group=" << group;
  oss << ", button_number=" << button_number;
  oss << ", state=" << state;
  oss << ", using_overides=" << (using_overides ? "true" : "false");
  oss << ", pattern_override=" << pattern_override;
  oss << ", x_offset_override=" << x_offset_override;
  oss << ", y_offset_override=" << y_offset_override;
  return oss.str();
}
