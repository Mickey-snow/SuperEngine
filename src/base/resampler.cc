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

namespace zr = zita_resampler;

template <class T>
struct remove_cvref {
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

void Resampler::Resample(AudioData& in_data) {
  if (in_data.spec.sample_rate == target_frequency_)
    return;

  zr::Resampler impl;

  if (impl.setup(in_data.spec.sample_rate, target_frequency_, 1, 64)) {
    std::ostringstream os;
    os << "Sample rate ratio " << target_frequency_ << '/'
       << in_data.spec.sample_rate << " is not supported.";
    throw std::runtime_error(os.str());
  }

  float *in_buf, *out_buf;
  int in_size, out_size;
  std::visit(
      [&](auto& input_sample) {
        in_size = input_sample.size();
        out_size = 1.05 * input_sample.size() / in_data.spec.sample_rate *
                   target_frequency_;
        in_buf = new float[in_size];
        out_buf = new float[out_size];
        using container_t = remove_cvref_t<decltype(input_sample)>;
        if (std::is_floating_point_v<typename container_t::value_type>)
          std::copy(input_sample.cbegin(), input_sample.cend(), in_buf);
        else
          std::transform(
              input_sample.cbegin(), input_sample.cend(), in_buf,
              [](const auto& value) -> float {
                using T = remove_cvref_t<decltype(value)>;
                return 1.0 * value /
                       static_cast<double>(std::numeric_limits<T>::max());
              });

        input_sample.clear();
      },
      in_data.data);

  impl.inp_count = in_size;
  impl.out_count = out_size;
  impl.inp_data = in_buf;
  impl.out_data = out_buf;
  impl.process();

  if (impl.inp_count != 0)
    throw std::runtime_error("Resampler error");

  out_size -= impl.out_count;
  std::visit(
      [out_size, out_buf](auto& output_sample) {
        output_sample.reserve(out_size);
        using T = remove_cvref_t<decltype(output_sample)>;
        for (int i = 0; i < out_size; ++i)
          if constexpr (std::is_floating_point_v<typename T::value_type>)
            output_sample.push_back(out_buf[i]);
          else
            output_sample.push_back(
                out_buf[i] *
                static_cast<double>(
                    std::numeric_limits<typename T::value_type>::max()));
      },
      in_data.data);

  delete[] in_buf;
  delete[] out_buf;

  in_data.spec.sample_rate = target_frequency_;
}
