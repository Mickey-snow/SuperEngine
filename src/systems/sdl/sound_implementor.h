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

#include "systems/base/isound_system.h"

#include <cstdint>
#include <memory>
#include <vector>

class SDLSoundImpl : public ISoundSystem {
 public:
  SDLSoundImpl();
  ~SDLSoundImpl();

  virtual void InitSystem() const override;
  virtual void QuitSystem() const override;

  virtual void AllocateChannels(int num) const override;
  virtual void OpenAudio(AVSpec spec, int buffer_size) const override;
  virtual void CloseAudio() const override;

  virtual int FindIdleChannel() const override;
  virtual void SetVolume(int channel, int vol) const override;
  virtual bool IsPlaying(int channel) const override;
  virtual int PlayChannel(int channel, player_t audio) override;
  virtual int FadeOutChannel(int channel, int fadetime) const override;
  virtual void HaltChannel(int channel) const override;
  virtual void HaltAllChannels() const override;

  virtual void PlayBgm(player_t audio) override;
  virtual player_t GetBgm() const override;
  virtual void EnableBgm() override;
  virtual void DisableBgm() override;

  uint16_t ToSDLSoundFormat(AV_SAMPLE_FMT fmt) const;
  AV_SAMPLE_FMT FromSDLSoundFormat(uint16_t fmt) const;

 private:
  const char* GetError() const;

  static void OnChannelFinished(int channel);             // callback
  static void OnMusic(void*, uint8_t* buffer, int size);  // callback

  class SDLSoundChunk;
  struct ChannelInfo {
    player_t player;
    SDLSoundImpl* implementor;
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
