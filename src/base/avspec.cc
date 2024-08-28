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
//
// -----------------------------------------------------------------------

#include "base/avspec.h"

#include <string>

std::string to_string(AV_SAMPLE_FMT fmt) {
  switch (fmt) {
    case AV_SAMPLE_FMT::NONE:
      return "NONE";
    case AV_SAMPLE_FMT::U8:
      return "U8";
    case AV_SAMPLE_FMT::S16:
      return "S16";
    case AV_SAMPLE_FMT::S32:
      return "S32";
    case AV_SAMPLE_FMT::S64:
      return "S64";
    case AV_SAMPLE_FMT::FLT:
      return "FLT";
    case AV_SAMPLE_FMT::DBL:
      return "DBL";
    default:
      return "UNKNOWN";
  }
}

bool AVChannelLayout::operator==(AVChannelLayout rhs) const {
  return channel_count == rhs.channel_count &&
         channel_order == rhs.channel_order;
}

bool AVSpec::operator==(const AVSpec& rhs) const {
  return sample_rate == rhs.sample_rate && sample_format == rhs.sample_format &&
         channel_layout == rhs.channel_layout;
}
