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

#include "base/audio_player.h"
#include "base/avspec.h"

class SoundSystemImpl {
 public:
  SoundSystemImpl() = default;
  ~SoundSystemImpl() = default;

  virtual void InitSystem() const;
  virtual void QuitSystem() const;

  virtual void AllocateChannels(int num) const;
  virtual void OpenAudio(AVSpec spec, int buffer_size) const;
  virtual void CloseAudio() const;
  virtual AVSpec QuerySpec() const;

  virtual int FindIdleChannel() const;
  virtual void SetVolume(int channel, int vol) const;
  virtual bool IsPlaying(int channel) const;
  virtual int PlayChannel(int channel, player_t audio);
  virtual int FadeOutChannel(int channel, int fadetime) const;
  virtual void HaltChannel(int channel) const;
  virtual void HaltAllChannels() const;

  virtual void PlayBgm(player_t audio);
  virtual player_t GetBgm() const;
  virtual void EnableBgm();
  virtual void DisableBgm();

  uint16_t ToSDLSoundFormat(AV_SAMPLE_FMT fmt) const;
  AV_SAMPLE_FMT FromSDLSoundFormat(uint16_t fmt) const;

 private:
  const char* GetError() const;

  static void OnChannelFinished(int channel);             // callback
  static void OnMusic(void*, uint8_t* buffer, int size);  // callback

  class SDLSoundChunk;
  struct ChannelInfo {
    player_t player;
    SoundSystemImpl* implementor;
    std::vector<uint8_t> buffer;
    std::unique_ptr<SDLSoundChunk> chunk;

    bool IsIdle() const;
    void Reset();
  };

  static std::vector<ChannelInfo> ch_;
  static player_t bgm_player_;
  static bool bgm_enabled_;
  static AVSpec spec_;
};

#endif
