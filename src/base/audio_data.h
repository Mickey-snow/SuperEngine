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

#ifndef SRC_BASE_AUDIO_DATA_H_
#define SRC_BASE_AUDIO_DATA_H_

#include "base/avspec.h"

#include <variant>
#include <vector>
#include <stdexcept>

using avsample_buffer_t = std::variant<std::vector<avsample_u8_t>,
                                       std::vector<avsample_s8_t>,
                                       std::vector<avsample_s16_t>,
                                       std::vector<avsample_s32_t>,
                                       std::vector<avsample_s64_t>,
                                       std::vector<avsample_flt_t>,
                                       std::vector<avsample_dbl_t>>;

struct AudioData {
  AVSpec spec;
  avsample_buffer_t data;

  AudioData Slice(int fr, int to, int step = 0);

  // Initializes the audio data buffer based on spec.sample_format.
  void PrepareDatabuf();
  void Clear() { PrepareDatabuf(); }

  size_t SampleCount() const;

  AudioData& Append(const AudioData& rhs);
  AudioData& Append(AudioData&& rhs);

  template <typename... Ts>
  static AudioData Concat(Ts&&... params) {
    static_assert((sizeof...(Ts) > 0), "Parameter pack must not be empty");
    static_assert(((std::is_same_v<std::decay_t<Ts>, AudioData>) && ...),
                  "All parameters must be of type AudioData.");

    auto audio_data_list =
        std::tuple<std::decay_t<Ts>...>(std::forward<Ts>(params)...);
    const auto& first_spec = std::get<0>(audio_data_list).spec;
    const auto& first_data = std::get<0>(audio_data_list).data;

    auto check_specs = [&](const auto& audio_data) {
      if (!(audio_data.spec == first_spec))
        throw std::invalid_argument(
            "All AudioData objects must have the same AVSpec.");
    };
    std::apply([&](auto&&... x) { (check_specs(x), ...); }, audio_data_list);

    auto check_data_type = [&](const auto& audio_data) {
      if (audio_data.data.index() != first_data.index()) {
        throw std::invalid_argument(
            "All AudioData objects must have the same data type.");
      }
    };
    std::apply([&](auto&&... x) { (check_data_type(x), ...); },
               audio_data_list);

    avsample_buffer_t concat_data;
    std::visit(
        [&](auto&& first_buffer) {
          using sample_buf_t = std::decay_t<decltype(first_buffer)>;
          sample_buf_t concat_buf;

          auto do_concat = [&](const auto& audio_data) {
            const auto& data = std::get<sample_buf_t>(audio_data.data);
            concat_buf.insert(concat_buf.end(), data.begin(), data.end());
          };
          std::apply([&](auto&&... x) { (do_concat(x), ...); },
                     audio_data_list);

          concat_data = std::move(concat_buf);
        },
        first_data);

    return AudioData{.spec = first_spec, .data = std::move(concat_data)};
  }
};

#endif
