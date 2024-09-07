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

class AudioPlayer {
 public:
  using time_ms_t = long long;
  using sample_count_t = long long;

  AudioPlayer(AudioDecoder&& dec);
  time_ms_t GetCurrentTime() const;
  AudioData LoadPCM(sample_count_t nsamples);
  AudioData LoadRemain();
  bool IsLoopingEnabled() const;
  void SetLooping(bool loop);
  bool IsPlaying() const;
  void FadeIn(time_ms_t ms);
  void FadeOut(time_ms_t ms);
  void Terminate();
  AVSpec GetSpec() const;

 private:
  void OnEndOfPlayback();
  void EnsureBufferCapacity(size_t size);
  void PushEmpty(size_t size);
  AudioData PopFront(size_t size);
  sample_count_t TimeToSampleCount(time_ms_t time);
  time_ms_t SampleCountToTime(sample_count_t samples);

  AudioDecoder decoder_;
  bool should_loop_;
  bool is_active_;
  long long pcm_cur_;

  std::deque<AudioData> buf_;
  size_t buf_size_;
};

using player_t = std::shared_ptr<AudioPlayer>;

template <typename... Ts>
player_t CreateAudioPlayer(Ts&&... params) {
  AudioDecoder decoder(std::forward<Ts>(params)...);
  return std::make_shared<AudioPlayer>(std::move(decoder));
}

#endif
