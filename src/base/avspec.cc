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

#include <stdexcept>
#include <string>

std::string to_string(AV_SAMPLE_FMT fmt) {
  switch (fmt) {
    case AV_SAMPLE_FMT::NONE:
      return "NONE";
    case AV_SAMPLE_FMT::U8:
      return "U8";
    case AV_SAMPLE_FMT::S8:
      return "S8";
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

bool AVSpec::operator==(const AVSpec& rhs) const {
  return sample_rate == rhs.sample_rate && sample_format == rhs.sample_format &&
         channel_count == rhs.channel_count;
}

void AudioData::PrepareDatabuf() {
  switch (spec.sample_format) {
    case AV_SAMPLE_FMT::U8:
      data = std::vector<avsample_u8_t>{};
      break;
    case AV_SAMPLE_FMT::S8:
      data = std::vector<avsample_s8_t>{};
      break;
    case AV_SAMPLE_FMT::S16:
      data = std::vector<avsample_s16_t>{};
      break;
    case AV_SAMPLE_FMT::S32:
      data = std::vector<avsample_s32_t>{};
      break;
    case AV_SAMPLE_FMT::S64:
      data = std::vector<avsample_s64_t>{};
      break;
    case AV_SAMPLE_FMT::FLT:
      data = std::vector<avsample_flt_t>{};
      break;
    case AV_SAMPLE_FMT::DBL:
      data = std::vector<avsample_dbl_t>{};
      break;
    default:
      // Handle AV_SAMPLE_FMT::NONE
      throw std::runtime_error("Unsupported audio sample format");
  }
}

size_t AudioData::SampleCount() const {
  return std::visit([](const auto& data) -> size_t { return data.size(); },
                    data);
}
