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

#include "base/avspec.h"

struct Mix_Chunk;
using Chunk_t = Mix_Chunk;

class SoundSystemImpl {
 public:
  SoundSystemImpl() = default;
  ~SoundSystemImpl() = default;

  virtual void InitSystem() const;
  virtual void QuitSystem() const;
  virtual void AllocateChannels(const int& num) const;
  virtual int OpenAudio(int rate,
                        AV_SAMPLE_FMT format,
                        int channels,
                        int buffers) const;
  virtual void CloseAudio() const;
  virtual AVSpec QuerySpec() const;
  virtual void ChannelFinished(void (*callback)(int)) const;
  virtual void SetVolume(int channel, int vol) const;
  virtual int Playing(int channel) const;
  virtual void HookMusic(void (*callback)(void*, uint8_t*, int),
                         void* data) const;
  virtual void MixAudio(uint8_t* dst, uint8_t* src, int len, int volume) const;
  virtual int MaxVolumn() const;
  virtual int PlayChannel(int channel, Chunk_t* chunk, int loops) const;
  virtual int FadeInChannel(int channel,
                            Mix_Chunk* chunk,
                            int loops,
                            int ms) const;
  virtual int FadeOutChannel(int channel, int fadetime) const;
  virtual void HaltChannel(int channel) const;
  virtual Chunk_t* LoadWAV_RW(char* data, int length) const;
  virtual Chunk_t* LoadWAV(const char* path) const;
  virtual void FreeChunk(Chunk_t* chunk) const;
  virtual const char* GetError() const;

  uint16_t ToSDLSoundFormat(AV_SAMPLE_FMT fmt) const;
  AV_SAMPLE_FMT FromSDLSoundFormat(uint16_t fmt) const;
};

#endif
