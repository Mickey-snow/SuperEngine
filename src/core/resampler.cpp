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

#include "core/resampler.hpp"

#include "samplerate.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

static const auto clamp_flt = [](float& x) { x = std::clamp(x, -1.0f, 1.0f); };

// -----------------------------------------------------------------------
// class srcResampler
// -----------------------------------------------------------------------

srcResampler::srcResampler(int freq) : target_frequency_(freq) {}
srcResampler::~srcResampler() = default;

void srcResampler::Resample(AudioData& pcm) {
  const double ratio =
      static_cast<double>(target_frequency_) / pcm.spec.sample_rate;
  const auto channels = pcm.spec.channel_count;
  auto input = pcm.GetAs<float>();
  std::vector<float> output(input.size() * ratio + 1024, 0);

  SRC_DATA src_data;
  src_data.data_in = input.data();
  src_data.input_frames = input.size() / channels;
  src_data.data_out = output.data();
  src_data.output_frames = output.size() / channels;
  src_data.src_ratio = ratio;
  static constexpr auto src_quality = SRC_SINC_FASTEST;

  int error_code;
  if ((error_code = src_simple(&src_data, src_quality, channels)) != 0) {
    using std::string_literals::operator""s;
    throw std::runtime_error("srcResampler: error converting samples. "s +
                             src_strerror(error_code));
  }

  if (src_data.input_frames_used * channels != input.size()) {
    std::ostringstream oss;
    oss << "srcResampler: resample incomplete. (";
    oss << src_data.input_frames_used * channels;
    oss << " out of " << input.size() << " converted)";
    throw std::runtime_error(oss.str());
  }

  output.resize(src_data.output_frames_gen * channels);
  std::for_each(output.begin(), output.end(), clamp_flt);

  pcm.spec.sample_rate = target_frequency_;
  pcm.spec.sample_format = AV_SAMPLE_FMT::FLT;
  pcm.data = std::move(output);
}
