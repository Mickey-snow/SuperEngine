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

#ifndef SRC_SYSTEMS_SDL_SOUND_IMPLEMENTOR_H_
#define SRC_SYSTEMS_SDL_SOUND_IMPLEMENTOR_H_

#include <cstdint>
#include <tuple>

#include "base/audio_player.h"
#include "base/avdec/wav.h"
#include "base/avspec.h"

struct Mix_Chunk;
using Chunk_t = Mix_Chunk;

class SoundSystemImpl {
 public:
  using player_t = std::shared_ptr<AudioPlayer>;

  SoundSystemImpl() = default;
  ~SoundSystemImpl() = default;

  virtual void InitSystem() const;
  virtual void QuitSystem() const;
  virtual void AllocateChannels(int num) const;
  virtual int OpenAudio(int rate,
                        AV_SAMPLE_FMT format,
                        int channels,
                        int buffers) const;
  virtual void CloseAudio() const;
  virtual AVSpec QuerySpec() const;
  virtual void SetVolume(int channel, int vol) const;
  virtual bool IsPlaying(int channel) const;
  virtual void HookMusic(void (*callback)(void*, uint8_t*, int),
                         void* data) const;
  virtual void MixAudio(uint8_t* dst, uint8_t* src, int len, int volume) const;
  virtual int MaxVolume() const;

  virtual int FindIdleChannel() const;
  virtual int PlayChannel(int channel, std::shared_ptr<AudioPlayer> audio);
  virtual int PlayBgm(player_t audio);

  virtual int FadeOutChannel(int channel, int fadetime) const;
  virtual void HaltChannel(int channel) const;
  virtual void HaltAllChannels() const;
  virtual const char* GetError() const;

  uint16_t ToSDLSoundFormat(AV_SAMPLE_FMT fmt) const;
  AV_SAMPLE_FMT FromSDLSoundFormat(uint16_t fmt) const;

 private:
  static void OnChannelFinished(int channel); // callback

  struct ChannelInfo {
    player_t player;
    SoundSystemImpl* implementor;
    std::vector<uint8_t> buffer;
    void* chunk;

    bool IsIdle() const;
    void Reset();
  };

  static std::vector<ChannelInfo> ch_;
};

#endif
