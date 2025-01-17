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

#pragma once

#include "core/avspec.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <variant>
#include <vector>

#ifdef PARALLEL
#include <execution>
namespace execution = std::execution;
#else
enum class execution { seq, unseq, par_unseq, par };
#endif

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

  template <typename T>
  std::vector<T>* Get() {
    if (auto ptr = std::get_if<std::vector<T>>(&data)) {
      return ptr;
    }
    return nullptr;
  }

  template <typename T>
  const std::vector<T>* Get() const {
    return const_cast<const std::vector<T>*>(Get<T>());
  }

  template <typename T>
  void Apply(std::function<void(T&)> fn) {
    std::visit(
        [&fn](auto& data) {
#ifdef PARALLEL
          std::for_each(execution::par_unseq, data.begin(), data.end(), fn);
#else
          std::for_each(data.begin(), data.end(), fn);
#endif
        },
        data);
  }

  AudioData Slice(int fr, int to, int step = 0);

  template <typename OutType>
  std::vector<OutType> GetAs() const {
    [[maybe_unused]] auto scale_to_float = [](auto sample) -> double {
      using T = std::decay_t<decltype(sample)>;

      constexpr double min = static_cast<double>(std::numeric_limits<T>::min());
      constexpr double max = static_cast<double>(std::numeric_limits<T>::max());

      if constexpr (std::is_unsigned_v<T>)
        return (static_cast<double>(sample) - min) / (max - min) * 2.0 - 1.0;
      else
        return static_cast<double>(sample) / (sample < 0 ? -min : max);
    };

    [[maybe_unused]] auto scale_to_int = [](double sample) -> OutType {
      constexpr double min =
          static_cast<double>(std::numeric_limits<OutType>::min());
      constexpr double max =
          static_cast<double>(std::numeric_limits<OutType>::max());

      if constexpr (std::is_unsigned_v<OutType>)
        return static_cast<OutType>((sample + 1.0) / 2.0 * (max - min) + min);
      else
        return static_cast<OutType>(sample * (sample < 0 ? -min : max));
    };

    return std::visit(
        [&](auto& data) -> std::vector<OutType> {
          using container_t = std::decay_t<decltype(data)>;
          using InType = typename container_t::value_type;

          std::vector<OutType> result;
          result.reserve(data.size());
          for (const auto& sample : data) {
            if constexpr (std::is_same_v<InType, OutType>)
              result.push_back(sample);

            else if constexpr (std::is_floating_point<InType>::value &&
                               std::is_integral<OutType>::value) {
              if (sample < -1.0 || sample > 1.0) {
                throw std::out_of_range(
                    "Floating point samples should be within range [-1.0,1.0], "
                    "got: " +
                    std::to_string(sample));
              }

              result.push_back(scale_to_int(sample));
            } else if constexpr (std::is_integral<InType>::value &&
                                 std::is_floating_point<OutType>::value) {
              result.push_back(static_cast<OutType>(scale_to_float(sample)));
            } else if constexpr (std::is_integral<InType>::value &&
                                 std::is_integral<OutType>::value) {
              result.push_back(scale_to_int(scale_to_float(sample)));
            } else {
              // Default case for floating-point to floating-point
              result.push_back(static_cast<OutType>(sample));
            }
          }

          return result;
        },
        data);
  }

  avsample_buffer_t GetAs(AV_SAMPLE_FMT fmt) const;

  // Initializes the audio data buffer based on spec.sample_format.
  void PrepareDatabuf();
  void Clear() { PrepareDatabuf(); }

  size_t SampleCount() const;
  size_t ByteLength() const;

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
