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

#include "base/audio_player.h"

#include <stdexcept>

AudioPlayer::AudioPlayer(AudioDecoder&& dec)
    : decoder_(std::move(dec)),
      should_loop_(false),
      is_active_(decoder_.HasNext()),
      pcm_cur_(0),
      buf_size_(0) {}

AudioPlayer::time_ms_t AudioPlayer::GetCurrentTime() const {
  auto spec = GetSpec();
  return pcm_cur_ * 1000 / (spec.sample_rate * spec.channel_count);
}

AudioData AudioPlayer::LoadPCM(sample_count_t nsamples) {
  if (nsamples <= 0)
    throw std::invalid_argument("Samples should be greater than 0, got: " +
                                std::to_string(nsamples));
  EnsureBufferCapacity(nsamples);

  AudioData ret;
  while (ret.SampleCount() < nsamples ||
         (!buf_.empty() && buf_.front().SampleCount() == 0)) {
    auto front = PopFront(nsamples - ret.SampleCount());
    auto front_samples = front.SampleCount();
    if (front_samples == 0) {
      pcm_cur_ = 0;
      continue;
    }

    pcm_cur_ += front_samples;
    ret.Append(std::move(front));
  }

  return ret;
}

AudioData AudioPlayer::LoadRemain() {
  AudioData ret;
  while (!buf_.empty()) {
    const auto& front = buf_.front();
    if (front.SampleCount() == 0)
      return ret;
    ret.Append(front);
    buf_size_ -= front.SampleCount();
    buf_.pop_front();
  }

  while (decoder_.HasNext() && is_active_)
    ret.Append(decoder_.DecodeNext());
  return ret;
}

bool AudioPlayer::IsLoopingEnabled() const { return should_loop_; }

void AudioPlayer::SetLooping(bool loop) {
  should_loop_ = loop;
  if (!loop) {
    buf_size_ = 0;
    for (auto it = buf_.begin(); it != buf_.end(); ++it) {
      if (it->SampleCount() == 0) {
        buf_.erase(it, buf_.end());
        OnEndOfPlayback();
        break;
      }
      buf_size_ += it->SampleCount();
    }
  }
}

bool AudioPlayer::IsPlaying() const { return is_active_ || buf_size_ > 0; }

void AudioPlayer::FadeIn(time_ms_t ms) {
  if (ms < 0)
    throw std::invalid_argument(
        "Fade duration must be greater than or equal to 0, got: " +
        std::to_string(ms));

  sample_count_t fade_samples = TimeToSampleCount(ms);
  EnsureBufferCapacity(fade_samples);
  for (int i = 0, chunk_id = 0; i < fade_samples; ++chunk_id) {
    auto& chunk = buf_.at(chunk_id);
    std::visit(
        [&](auto& pcm) {
          for (auto& it : pcm) {
            if (i >= fade_samples)
              return;
            float fade_factor = static_cast<float>(i++) / fade_samples;
            it *= fade_factor;
          }
        },
        chunk.data);
  }
}

void AudioPlayer::FadeOut(time_ms_t ms) {
  if (ms < 0)
    throw std::invalid_argument(
        "Fade duration must be greater than or equal to 0, got: " +
        std::to_string(ms));

  sample_count_t fade_samples = TimeToSampleCount(ms);
  EnsureBufferCapacity(fade_samples);

  if (buf_size_ > fade_samples) {
    auto discard_size = buf_size_ - fade_samples;
    std::visit(
        [&](auto& last) { last.erase(last.end() - discard_size, last.end()); },
        buf_.back().data);
    buf_size_ = fade_samples;
  }

  sample_count_t current_sample = 0;
  for (size_t i = 0; i < buf_.size(); ++i) {
    std::visit(
        [&current_sample, fade_samples](auto& data) {
          for (auto& pcm : data) {
            float fade_factor =
                1.0 - static_cast<float>(current_sample++) / fade_samples;
            pcm *= fade_factor;
          }
        },
        buf_[i].data);
  }

  Terminate();
}

void AudioPlayer::Terminate() { is_active_ = false; }

AVSpec AudioPlayer::GetSpec() const { return decoder_.GetSpec(); }

void AudioPlayer::OnEndOfPlayback() {
  if (should_loop_) {
    buf_.push_back(AudioData{});  // loop marker
    decoder_.Rewind();
  } else
    Terminate();
}

void AudioPlayer::EnsureBufferCapacity(size_t size) {
  while (buf_size_ < size && is_active_) {
    if (!decoder_.HasNext()) {
      OnEndOfPlayback();
      continue;
    }

    auto next_chunk = decoder_.DecodeNext();
    if (next_chunk.SampleCount() == 0)
      throw std::runtime_error("AudioPlayer error: Found empty pcm chunk");

    buf_.push_back(std::move(next_chunk));
    buf_size_ += buf_.back().SampleCount();
  }

  if (buf_size_ < size)
    PushEmpty(size - buf_size_);

  if (!decoder_.HasNext())
    OnEndOfPlayback();
}

void AudioPlayer::PushEmpty(size_t size) {
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
  buf_.push_back(std::move(tail));
  buf_size_ += size;
}

AudioData AudioPlayer::PopFront(size_t size) {
  AudioData ret;
  ret.spec = buf_.front().spec;

  if (buf_.front().SampleCount() <= size) {
    ret = buf_.front();
    buf_.pop_front();
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

static constexpr auto s_over_ms = 1000;

AudioPlayer::sample_count_t AudioPlayer::TimeToSampleCount(time_ms_t time) {
  auto spec = GetSpec();
  return time * spec.sample_rate * spec.channel_count / s_over_ms;
}

AudioPlayer::time_ms_t AudioPlayer::SampleCountToTime(sample_count_t samples) {
  auto spec = GetSpec();
  return samples * s_over_ms / (spec.sample_rate * spec.channel_count);
}
