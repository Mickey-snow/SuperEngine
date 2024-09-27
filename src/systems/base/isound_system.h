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

#ifndef SRC_SYSTEMS_BASE_ISOUND_SYSTEM_H_
#define SRC_SYSTEMS_BASE_ISOUND_SYSTEM_H_

#include "base/audio_player.h"
#include "base/avspec.h"

class ISoundSystem {
 public:
  virtual ~ISoundSystem() = default;

  virtual void InitSystem() const = 0;
  virtual void QuitSystem() const = 0;

  virtual void AllocateChannels(int num) const = 0;
  virtual void OpenAudio(AVSpec spec, int buffer_size) const = 0;
  virtual void CloseAudio() const = 0;

  virtual int FindIdleChannel() const = 0;
  virtual void SetVolume(int channel, int vol) const = 0;
  virtual bool IsPlaying(int channel) const = 0;
  virtual int PlayChannel(int channel, player_t audio) = 0;
  virtual int FadeOutChannel(int channel, int fadetime) const = 0;
  virtual void HaltChannel(int channel) const = 0;
  virtual void HaltAllChannels() const = 0;

  virtual void PlayBgm(player_t audio) = 0;
  virtual player_t GetBgm() const = 0;
  virtual void EnableBgm() = 0;
  virtual void DisableBgm() = 0;
};

#endif
