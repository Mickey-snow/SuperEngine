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

#include "zita-resampler/resampler.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

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

void zitaResampler::Resample(AudioData& in_data) {
  if (in_data.spec.sample_rate == target_frequency_)
    return;

  zr::Resampler& impl = data_->impl;

  const auto hlen = 96;
  const auto channels = in_data.spec.channel_count;
  if (impl.setup(in_data.spec.sample_rate, target_frequency_, channels, hlen)) {
    std::ostringstream os;
    os << "Sample rate ratio " << target_frequency_ << '/'
       << in_data.spec.sample_rate << " is not supported.";
    throw std::runtime_error(os.str());
  }

  auto in_pcm = in_data.GetAs<float>();
  std::vector<float> out_pcm(
      in_pcm.size() * target_frequency_ / in_data.spec.sample_rate + 1024, 0);

  impl.inp_count = in_pcm.size();
  impl.out_count = out_pcm.size();
  impl.inp_data = in_pcm.data();
  impl.out_data = out_pcm.data();
  impl.process();

  if (impl.inp_count != 0)
    throw std::runtime_error("Resampler error");

  out_pcm.resize(out_pcm.size() - impl.out_count);
  in_data.spec.sample_rate = target_frequency_;
  in_data.data = std::move(out_pcm);
  in_data.data = in_data.GetAs(in_data.spec.sample_format);
}
