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

#include "base/audio_data.h"

AudioData AudioData::Slice(int fr, int to, int step) {
  if (fr < 0)
    fr = SampleCount() + fr;
  if (to < 0)
    to = SampleCount() + to;

  if (fr < 0 || fr >= SampleCount() || to < 0 || to > SampleCount())
    throw std::out_of_range("Index out of range");

  if (step == 0)
    step = (fr < to) ? 1 : -1;

  avsample_buffer_t slicedData = std::visit(
      [&](const auto& buf) -> avsample_buffer_t {
        using container_t = std::decay_t<decltype(buf)>;
        container_t result;
        if (step > 0)
          for (int i = fr; i < to; i += step)
            result.push_back(buf[i]);
        else
          for (int i = fr; i > to; i += step)
            result.push_back(buf[i]);
        return result;
      },
      data);
  return {spec, std::move(slicedData)};
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

size_t AudioData::ByteLength() const {
  auto sample_count = SampleCount();
  if (sample_count == 0)
    return 0;
  return sample_count * Bytecount(spec.sample_format);
}

AudioData& AudioData::Append(const AudioData& rhs) {
  if (SampleCount() == 0)
    return *this = rhs;
  else if (rhs.SampleCount() == 0)
    return *this;
  else
    return *this = AudioData::Concat(std::move(*this), rhs);
}

AudioData& AudioData::Append(AudioData&& rhs) {
  if (SampleCount() == 0)
    return *this = std::move(rhs);
  else if (rhs.SampleCount() == 0)
    return *this;
  else
    return *this = AudioData::Concat(std::move(*this), std::move(rhs));
}
