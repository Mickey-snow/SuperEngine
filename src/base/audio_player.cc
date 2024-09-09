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

using sample_count_t = AudioPlayer::sample_count_t;

// -----------------------------------------------------------------------

class VolumeAdjustCmd : public AudioPlayer::ICommand {
 public:
  VolumeAdjustCmd(float start_volume,
                  float end_volume,
                  sample_count_t fadein_samples)
      : start_volume_(start_volume),
        end_volume_(end_volume),
        samples_(fadein_samples),
        faded_samples_(0) {}
  void Execute(AudioPlayer::AudioFrame& af) override {
    std::visit(
        [&](auto& data) {
          for (auto& it : data) {
            if (faded_samples_ >= samples_)
              break;
            auto fade_factor = static_cast<float>(faded_samples_++) / samples_;
            it *= start_volume_ + fade_factor * (end_volume_ - start_volume_);
          }
        },
        af.ad.data);
  }
  bool IsFinished() override { return faded_samples_ >= samples_; }

 private:
  float start_volume_, end_volume_;
  sample_count_t samples_, faded_samples_;
};

// -----------------------------------------------------------------------

AudioPlayer::AudioPlayer(AudioDecoder&& dec)
    : decoder_(std::move(dec)),
      loop_fr_(),
      loop_to_(),
      is_active_(decoder_.HasNext()),
      spec(decoder_.GetSpec()),
      buffer_() {}

AudioPlayer::time_ms_t AudioPlayer::GetCurrentTime() const {
  long long location;
  if (buffer_.has_value())
    location = buffer_->cur;
  else
    location = decoder_.Tell();

  return location * 1000 / spec.sample_rate;
}

AudioData AudioPlayer::LoadPCM(sample_count_t nsamples) {
  if (nsamples <= 0)
    throw std::invalid_argument("Samples should be greater than 0, got: " +
                                std::to_string(nsamples));

  AudioData ret;
  ret.spec = spec;
  ret.PrepareDatabuf();

  if (buffer_.has_value()) {
    ret.Append(buffer_->ad);
    buffer_.reset();
  }

  auto cur = decoder_.Tell();
  while (ret.SampleCount() < nsamples) {
    auto next = LoadNext();
    if (next.SampleCount() == 0)
      break;
    ClipFrame(next);
    cur = next.cur + next.SampleCount() / spec.channel_count;
    ret.Append(std::move(next.ad));
  }

  if (ret.SampleCount() > nsamples) {
    std::visit(
        [&](auto& data) {
          const auto n = data.size() - nsamples;
          cur -= n / spec.channel_count;

          using container_t = std::decay_t<decltype(data)>;
          container_t buf(data.end() - n, data.end());
          buffer_ = AudioFrame{.ad = {spec, std::move(buf)}, .cur = cur};
          data.erase(data.end() - n, data.end());
        },
        ret.data);
  }

  nsamples -= ret.SampleCount();
  if (nsamples > 0) {
    std::visit(
        [&nsamples](auto& data) {
          using container_t = std::decay_t<decltype(data)>;
          using value_t = typename container_t::value_type;
          value_t silent = value_t();
          if constexpr (std::is_unsigned_v<value_t>)
            silent =
                static_cast<value_t>(std::numeric_limits<value_t>::max() / 2);
          while (nsamples--)
            data.push_back(silent);
        },
        ret.data);
  }

  return ret;
}

AudioData AudioPlayer::LoadRemain() {
  auto cur = decoder_.Tell();
  AudioData ret;

  if (buffer_.has_value()) {
    cur = buffer_->cur;
    ret = buffer_->ad;
    buffer_.reset();
  }

  while (true) {
    auto next = LoadNext();
    if (next.SampleCount() == 0)
      break;
    if (next.cur < cur) {
      buffer_ = std::move(next);
      break;
    }

    ClipFrame(next);
    ret.Append(std::move(next.ad));
  }

  return ret;
}

bool AudioPlayer::IsLoopingEnabled() const { return loop_fr_.has_value(); }

void AudioPlayer::SetLoop(size_t loop_fr, size_t loop_to) {
  if (loop_fr >= loop_to) {
    std::ostringstream oss;
    oss << "Loop from (" << loop_fr << ')';
    oss << " must be less than loop to (" << loop_to << ").";
    throw std::invalid_argument(oss.str());
  }

  loop_fr_ = loop_fr;
  loop_to_ = loop_to;

  auto cur = buffer_.has_value() ? buffer_->cur : decoder_.Tell();
  if (cur < loop_fr || cur >= loop_to) {
    buffer_.reset();
    decoder_.Seek(loop_fr, SEEKDIR::BEG);
  }
}

void AudioPlayer::SetLooping(bool loop) {
  if (loop) {
    SetLoop(0, npos);
  } else {
    loop_fr_.reset();
    loop_to_.reset();
  }
}

bool AudioPlayer::IsPlaying() const { return is_active_; }

void AudioPlayer::Terminate() { is_active_ = false; }

void AudioPlayer::FadeIn(float fadein_ms) {
  auto fadein_samples = TimeToSampleCount(fadein_ms);
  cmd_.emplace_back(std::make_unique<VolumeAdjustCmd>(0, 1, fadein_samples));
}

void AudioPlayer::FadeOut(float fadeout_ms, bool should_then_terminate) {
  auto fadeout_samples = TimeToSampleCount(fadeout_ms);
  cmd_.emplace_back(std::make_unique<VolumeAdjustCmd>(1, 0, fadeout_samples));

  if (should_then_terminate) {
    class TerminateAfter : public ICommand {
     public:
      TerminateAfter(AudioPlayer& client, sample_count_t samples)
          : client_(client), samples_(samples) {}
      void Execute(AudioFrame& af) override {
        samples_ -= af.SampleCount();
        if (IsFinished())
          client_.Terminate();
      }
      bool IsFinished() override { return samples_ <= 0; }

     private:
      AudioPlayer& client_;
      sample_count_t samples_;
    };

    cmd_.emplace_back(std::make_unique<TerminateAfter>(*this, fadeout_samples));
  }
}

void AudioPlayer::OnEndOfPlayback() {
  size_t seek_to = loop_fr_.value_or(npos);
  if (seek_to != npos)
    decoder_.Seek(seek_to, SEEKDIR::BEG);
  else
    Terminate();
}

AudioPlayer::AudioFrame AudioPlayer::LoadNext() {
  if (!is_active_)
    return {};
  if (!decoder_.HasNext())
    return {};

  auto cur = decoder_.Tell();
  if (cur >= loop_to_.value_or(npos)) {
    decoder_.Seek(loop_fr_.value_or(0), SEEKDIR::BEG);
    cur = decoder_.Tell();
  }

  auto next_chunk = decoder_.DecodeNext();
  if (next_chunk.SampleCount() == 0)
    throw std::runtime_error("AudioPlayer error: Found empty pcm chunk");

  if (!decoder_.HasNext())
    OnEndOfPlayback();

  AudioFrame frame{.ad = std::move(next_chunk), .cur = std::move(cur)};
  for (auto it = cmd_.begin(); it != cmd_.end();) {
    if (it->get()->IsFinished()) {
      it = cmd_.erase(it);
      continue;
    }
    it->get()->Execute(frame);
    if (it->get()->IsFinished()) {
      it = cmd_.erase(it);
      continue;
    }
    ++it;
  }

  return frame;
}

void AudioPlayer::ClipFrame(AudioFrame& frame) const {
  long long frame_fr = frame.cur,
            frame_to = frame.cur + frame.SampleCount() / spec.channel_count;
  long long audio_fr = loop_fr_.value_or(0), audio_to = loop_to_.value_or(npos);

  if (frame_to < audio_fr || audio_to <= frame_fr) {
    std::visit([](auto& data) { data.clear(); }, frame.ad.data);
  } else if (frame_fr < audio_fr || audio_to < frame_to) {
    long long clip_front = std::max(0ll, audio_fr - frame_fr);
    long long clip_back = std::max(0ll, frame_to - audio_to);
    frame.cur += clip_front;
    std::visit(
        [&](auto& data) {
          clip_front *= spec.channel_count;
          clip_back *= spec.channel_count;
          using container_t = std::decay_t<decltype(data)>;
          container_t clipped_data(data.cbegin() + clip_front,
                                   data.cend() - clip_back);
          data = std::move(clipped_data);
        },
        frame.ad.data);
  }
}

static constexpr auto s_over_ms = 1000;

AudioPlayer::sample_count_t AudioPlayer::TimeToSampleCount(time_ms_t time) {
  return time * spec.sample_rate * spec.channel_count / s_over_ms;
}

AudioPlayer::time_ms_t AudioPlayer::SampleCountToTime(sample_count_t samples) {
  return samples * s_over_ms / (spec.sample_rate * spec.channel_count);
}
