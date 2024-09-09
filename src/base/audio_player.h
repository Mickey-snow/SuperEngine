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

#include <deque>
#include <list>

class AudioPlayer {
 public:
  using time_ms_t = long long;
  using sample_count_t = long long;

  static constexpr size_t npos = std::numeric_limits<int32_t>::max();

  AudioPlayer(AudioDecoder&& dec);

  time_ms_t GetCurrentTime() const;
  AudioData LoadPCM(sample_count_t nsamples);
  AudioData LoadRemain();
  bool IsLoopingEnabled() const;
  void SetLooping(bool loop);
  void SetLoop(size_t loop_fr, size_t loop_to);
  bool IsPlaying() const;
  void Terminate();

  void FadeIn(float fadein_ms);

private:
  struct AudioFrame {
    AudioData ad;
    long long cur;
    size_t SampleCount() const { return ad.SampleCount(); }
  };

  void OnEndOfPlayback();
  AudioFrame LoadNext();
  void ClipFrame(AudioFrame&) const;

  AudioDecoder decoder_;
  std::optional<size_t> loop_fr_, loop_to_;
  bool is_active_;
  AVSpec spec;
  std::optional<AudioFrame> buffer_;

 public:
  class ICommand {
   public:
    virtual ~ICommand() = default;
    virtual void Execute(AudioFrame&) = 0;
    virtual bool IsFinished() = 0;
  };

 private:
  std::list<std::unique_ptr<ICommand>> cmd_;

  sample_count_t TimeToSampleCount(time_ms_t time);
  time_ms_t SampleCountToTime(sample_count_t samples);
};

using player_t = std::shared_ptr<AudioPlayer>;

template <typename... Ts>
player_t CreateAudioPlayer(Ts&&... params) {
  AudioDecoder decoder(std::forward<Ts>(params)...);
  return std::make_shared<AudioPlayer>(std::move(decoder));
}

#endif
