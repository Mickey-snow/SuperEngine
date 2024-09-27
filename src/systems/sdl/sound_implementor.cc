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

#include "sound_implementor.h"

#include "base/resampler.h"

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include <stdexcept>
#include <string>

using std::string_literals::operator""s;

// -----------------------------------------------------------------------

class SDLAudioLocker {
 public:
  SDLAudioLocker() { SDL_LockAudio(); }
  ~SDLAudioLocker() { SDL_UnlockAudio(); }
};

// -----------------------------------------------------------------------

class SDLSoundImpl::SDLSoundChunk {
 public:
  SDLSoundChunk(Mix_Chunk* chunk) : chunk_(chunk) {}
  ~SDLSoundChunk() { Mix_FreeChunk(chunk_); }

  Mix_Chunk* Get() const noexcept { return chunk_; }

 private:
  Mix_Chunk* chunk_;
};

// -----------------------------------------------------------------------

bool SDLSoundImpl::ChannelInfo::IsIdle() const {
  return implementor == nullptr;
}

void SDLSoundImpl::ChannelInfo::Reset() {
  player = nullptr;
  implementor = nullptr;
  buffer.clear();
  chunk = nullptr;
}

// -----------------------------------------------------------------------

SDLSoundImpl::SDLSoundImpl() = default;

SDLSoundImpl::~SDLSoundImpl() = default;

void SDLSoundImpl::InitSystem() const { SDL_InitSubSystem(SDL_INIT_AUDIO); }

void SDLSoundImpl::QuitSystem() const { SDL_QuitSubSystem(SDL_INIT_AUDIO); }

void SDLSoundImpl::AllocateChannels(int num) const {
  Mix_AllocateChannels(num);
  ch_.resize(num);
  Mix_ChannelFinished(&SDLSoundImpl::OnChannelFinished);
}

void SDLSoundImpl::OpenAudio(AVSpec spec, int buf_size) const {
  if (Mix_OpenAudio(spec.sample_rate, ToSDLSoundFormat(spec.sample_format),
                    spec.channel_count, buf_size) == -1) {
    throw std::runtime_error("SDL Error: "s + GetError());
  }

  spec_ = spec;
  Mix_HookMusic(&SDLSoundImpl::OnMusic, NULL);
}

void SDLSoundImpl::CloseAudio() const {
  Mix_HookMusic(NULL, NULL);
  Mix_CloseAudio();
}

void SDLSoundImpl::SetVolume(int channel, int vol) const {
  Mix_Volume(channel, vol);
}

bool SDLSoundImpl::IsPlaying(int channel) const { return Mix_Playing(channel); }

int SDLSoundImpl::FindIdleChannel() const {
  if (ch_.empty())
    throw std::runtime_error("SDL Error: Channel not allocated.");

  for (int i = 0; i < ch_.size(); ++i)
    if (ch_[i].IsIdle())
      return i;

  throw std::runtime_error("All channels are busy.");
}

int SDLSoundImpl::PlayChannel(int channel, std::shared_ptr<AudioPlayer> audio) {
  AudioData audio_data = audio->LoadRemain();
  const auto system_frequency = spec_.sample_rate;
  if (audio_data.spec.sample_rate != system_frequency) {
    Resampler resampler(system_frequency);
    resampler.Resample(audio_data);
  }

  std::vector<uint8_t> pcm = EncodeWav(std::move(audio_data));
  Mix_Chunk* sound_chunk =
      Mix_LoadWAV_RW(SDL_RWFromMem(pcm.data(), pcm.size()), 1);
  if (!sound_chunk)
    throw std::runtime_error("SDL failed to create a new chunk.");

  ch_[channel] =
      (ChannelInfo){.player = audio,
                    .implementor = this,
                    .buffer = std::move(pcm),
                    .chunk = std::make_unique<SDLSoundChunk>(sound_chunk)};

  int ret = Mix_PlayChannel(channel, sound_chunk, 0);
  if (ret == -1) {
    ch_[channel].Reset();
    throw std::runtime_error("Failed to play on channel: " +
                             std::to_string(channel));
  }

  return ret;
}

void SDLSoundImpl::PlayBgm(player_t audio) {
  SDLAudioLocker lock;
  bgm_player_ = audio;
}

player_t SDLSoundImpl::GetBgm() const { return bgm_player_; }

void SDLSoundImpl::EnableBgm() { bgm_enabled_ = true; }

void SDLSoundImpl::DisableBgm() { bgm_enabled_ = false; }

int SDLSoundImpl::FadeOutChannel(int channel, int fadetime) const {
  return Mix_FadeOutChannel(channel, fadetime);
}

void SDLSoundImpl::HaltChannel(int channel) const {
  if (channel < 0) /* all channels */
    channel = -1;
  Mix_HaltChannel(channel);
}

void SDLSoundImpl::HaltAllChannels() const { HaltChannel(-1); }

const char* SDLSoundImpl::GetError() const { return Mix_GetError(); }

uint16_t SDLSoundImpl::ToSDLSoundFormat(AV_SAMPLE_FMT fmt) const {
  switch (fmt) {
    case AV_SAMPLE_FMT::U8:
      return AUDIO_U8;
    case AV_SAMPLE_FMT::S8:
      return AUDIO_S8;
    case AV_SAMPLE_FMT::S16:
      return AUDIO_S16SYS;
    case AV_SAMPLE_FMT::S32:
    case AV_SAMPLE_FMT::S64:
    case AV_SAMPLE_FMT::FLT:
    case AV_SAMPLE_FMT::DBL:
      throw std::invalid_argument("Unsupported SDL1.2 audio format for: " +
                                  to_string(fmt));

    default:
      throw std::invalid_argument("Invalid AV_SAMPLE_FMT format: " +
                                  std::to_string(static_cast<int>(fmt)));
  }
}

AV_SAMPLE_FMT SDLSoundImpl::FromSDLSoundFormat(uint16_t fmt) const {
  switch (fmt) {
    case AUDIO_U8:
      return AV_SAMPLE_FMT::U8;
    case AUDIO_S8:
      return AV_SAMPLE_FMT::S8;
    case AUDIO_S16SYS:
      return AV_SAMPLE_FMT::S16;
    default:
      throw std::invalid_argument("Invalid SDL audio format: " +
                                  std::to_string(fmt));
  }
}

void SDLSoundImpl::OnChannelFinished(int channel) {
  auto player = ch_[channel].player;
  auto implementor = ch_[channel].implementor;
  ch_[channel].Reset();

  if (!player || !implementor)
    return;
  if (player->IsPlaying())  // loop
    implementor->PlayChannel(channel, player);
}

void SDLSoundImpl::OnMusic(void*, uint8_t* stream, int len) {
  memset(stream, 0, len);

  if (!bgm_player_ || !bgm_enabled_)
    return;
  if (bgm_player_->GetStatus() == AudioPlayer::STATUS::TERMINATED) {
    bgm_player_ = nullptr;
    return;
  }

  // TODO: Implement logic for sample format other than 16bits width
  auto audio_data = bgm_player_->LoadPCM(len / 2);
  std::visit(
      [&](auto&& data) {
        memmove(stream, data.data(), len);
        data.clear();
      },
      std::move(audio_data.data));
}

std::vector<SDLSoundImpl::ChannelInfo> SDLSoundImpl::ch_;
player_t SDLSoundImpl::bgm_player_ = nullptr;
bool SDLSoundImpl::bgm_enabled_ = true;
AVSpec SDLSoundImpl::spec_;
