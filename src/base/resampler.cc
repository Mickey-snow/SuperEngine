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

#include "base/resampler.h"

#include "samplerate.h"
#include "zita-resampler/resampler.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

static const auto clamp_flt = [](float& x) { x = std::clamp(x, -1.0f, 1.0f); };

// -----------------------------------------------------------------------
// class zitaResampler
// -----------------------------------------------------------------------

namespace zr = zita_resampler;

struct zitaResampler::Context {
  zr::Resampler impl;
};

zitaResampler::zitaResampler(int freq)
    : target_frequency_(freq), data_(std::make_unique<Context>()) {}

zitaResampler::~zitaResampler() = default;

void zitaResampler::Resample(AudioData& pcm) {
  if (pcm.spec.sample_rate == target_frequency_)
    return;

  zr::Resampler& impl = data_->impl;

  const auto hlen = 96;
  const auto channels = pcm.spec.channel_count;
  if (impl.setup(pcm.spec.sample_rate, target_frequency_, channels, hlen)) {
    std::ostringstream os;
    os << "Sample rate ratio " << target_frequency_ << '/'
       << pcm.spec.sample_rate << " is not supported.";
    throw std::runtime_error(os.str());
  }

  auto in_pcm = pcm.GetAs<float>();
  std::vector<float> out_pcm(
      in_pcm.size() * target_frequency_ / pcm.spec.sample_rate + 1024, 0);

  impl.inp_count = in_pcm.size();
  impl.out_count = out_pcm.size();
  impl.inp_data = in_pcm.data();
  impl.out_data = out_pcm.data();
  impl.process();

  if (impl.inp_count != 0)
    throw std::runtime_error("Resampler error");

  out_pcm.resize(out_pcm.size() - impl.out_count);
  pcm.spec.sample_rate = target_frequency_;
  pcm.spec.sample_format = AV_SAMPLE_FMT::FLT;
  std::for_each(out_pcm.begin(), out_pcm.end(), clamp_flt);
  pcm.data = std::move(out_pcm);
}

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
  const auto src_quality = SRC_SINC_BEST_QUALITY;

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
