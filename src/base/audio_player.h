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

#ifndef SRC_BASE_AUDIO_PLAYER_H_
#define SRC_BASE_AUDIO_PLAYER_H_

#include "base/avdec/audio_decoder.h"
#include "base/avspec.h"

#include <queue>
#include <stdexcept>

class AudioPlayer {
 public:
  AudioPlayer(AudioDecoder&& dec)
      : decoder_(std::move(dec)),
        should_loop_(false),
        is_playing_(true),
        pcm_cur_(0) {}

  using time_ms_t = long long;
  time_ms_t GetCurrentTime() const {
    auto spec = GetSpec();
    return pcm_cur_ * 1000 / (spec.sample_rate * spec.channel_count);
  }

  using sample_count_t = long long;
  AudioData LoadPCM(sample_count_t nsamples) {
    if (nsamples <= 0)
      throw std::invalid_argument("Samples should be greater than 0, got: " +
                                  std::to_string(nsamples));
    Preserve(nsamples);

    AudioData ret;
    while (ret.SampleCount() < nsamples) {
      auto front = PopFront(nsamples - ret.SampleCount());
      auto front_samples = front.SampleCount();
      if (front_samples == 0) {
        pcm_cur_ = 0;
        continue;
      }

      pcm_cur_ += front_samples;
      ret.Concat(std::move(front));
    }

    return ret;
  }

  bool IsLoopingEnabled() const { return should_loop_; }
  void SetLooping(bool loop) { should_loop_ = loop; }

  bool IsPlaybackActive() const { return is_playing_; }

  void FadeIn(time_ms_t ms);

  void Fadeout(time_ms_t ms);

  void Terminate() { is_playing_ = false; }

  AVSpec GetSpec() const { return decoder_.GetSpec(); }

 private:
  void OnEndOfPlayback() {
    if (should_loop_) {
      buf_.push(AudioData{});  // loop marker
      decoder_.Rewind();
    } else
      Terminate();
  }

  void Preserve(size_t size) {
    while (buf_size_ < size && IsPlaybackActive()) {
      if (!decoder_.HasNext()) {
        OnEndOfPlayback();
        continue;
      }

      auto next_chunk = decoder_.DecodeNext();
      if (next_chunk.SampleCount() == 0)
        throw std::runtime_error("AudioPlayer error: Found empty pcm chunk");

      buf_.push(std::move(next_chunk));
      buf_size_ += buf_.back().SampleCount();
    }

    if (buf_size_ < size)
      PushEmpty(size - buf_size_);

    if (!decoder_.HasNext())
      OnEndOfPlayback();
  }

  void PushEmpty(size_t size) {
    AudioData tail;
    tail.spec = GetSpec();
    tail.PrepareDatabuf();
    std::visit(
        [size](auto& data) {
          using container_t = std::decay_t<decltype(data)>;
          using value_t = typename container_t::value_type;
          value_t silent = value_t();
          if constexpr (std::is_unsigned_v<value_t>)
            silent =
                static_cast<value_t>(std::numeric_limits<value_t>::max() / 2);
          for (size_t i = 0; i < size; ++i)
            data.push_back(silent);
        },
        tail.data);
    buf_.push(std::move(tail));
  }

  AudioData PopFront(size_t size) {
    AudioData ret;
    ret.spec = buf_.front().spec;

    if (buf_.front().SampleCount() <= size) {
      ret = buf_.front();
      buf_.pop();
    } else {
      ret.data = std::visit(
          [size](auto& front) -> avsample_buffer_t {
            using container_t = std::decay_t<decltype(front)>;
            container_t data(size);
            auto begin_it = front.begin();
            auto end_it = front.begin() + size;
            std::copy(begin_it, end_it, data.begin());
            front.erase(begin_it, end_it);
            return data;
          },
          buf_.front().data);
    }

    buf_size_ -= ret.SampleCount();
    return ret;
  }

  AudioDecoder decoder_;
  bool should_loop_, is_playing_;
  long long pcm_cur_;

  std::queue<AudioData> buf_;
  size_t buf_size_ = 0;
};

#endif
