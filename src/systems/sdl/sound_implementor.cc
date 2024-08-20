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

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

void SoundSystemImpl::InitSystem() const { SDL_InitSubSystem(SDL_INIT_AUDIO); }

void SoundSystemImpl::QuitSystem() const { SDL_QuitSubSystem(SDL_INIT_AUDIO); }

void SoundSystemImpl::AllocateChannels(const int& num) const {
  Mix_AllocateChannels(num);
}

int SoundSystemImpl::OpenAudio(int rate,
                               Uint16 format,
                               int channels,
                               int buffers) const {
  return Mix_OpenAudio(rate, format, channels, buffers);
}

void SoundSystemImpl::CloseAudio() const { Mix_CloseAudio(); }

std::tuple<int, Uint16, int> SoundSystemImpl::QuerySpec() const {
  int freq, channels;
  Uint16 format;
  Mix_QuerySpec(&freq, &format, &channels);
  return std::make_tuple(freq, format, channels);
}

void SoundSystemImpl::ChannelFinished(void (*callback)(int)) const {
  Mix_ChannelFinished(callback);
}

void SoundSystemImpl::SetVolume(int channel, int vol) const {
  Mix_Volume(channel, vol);
}

int SoundSystemImpl::Playing(int channel) const { return Mix_Playing(channel); }

void SoundSystemImpl::HookMusic(void (*callback)(void*, Uint8*, int),
                                void* data) const {
  Mix_HookMusic(callback, data);
}

void SoundSystemImpl::MixAudio(uint8_t* dst,
                               uint8_t* src,
                               int len,
                               int volume) const {
  SDL_MixAudio(dst, src, len, volume);
}

int SoundSystemImpl::MaxVolumn() const { return MIX_MAX_VOLUME; }

int SoundSystemImpl::PlayChannel(int channel, Chunk_t* chunk, int loops) const {
  return Mix_PlayChannel(channel, chunk, loops);
}

int SoundSystemImpl::FadeInChannel(int channel,
                                   Mix_Chunk* chunk,
                                   int loops,
                                   int ms) const {
  return Mix_FadeInChannel(channel, chunk, loops, ms);
}

int SoundSystemImpl::FadeOutChannel(int channel, int fadetime) const {
  return Mix_FadeOutChannel(channel, fadetime);
}

void SoundSystemImpl::HaltChannel(int channel) const {
  if (channel < 0) /* all channels */
    channel = -1;
  Mix_HaltChannel(channel);
}

Chunk_t* SoundSystemImpl::LoadWAV_RW(char* data, int length) const {
  auto rw = SDL_RWFromMem(data, length);
  return Mix_LoadWAV_RW(rw, 1);
}

Chunk_t* SoundSystemImpl::LoadWAV(const char* path) const {
  return Mix_LoadWAV(path);
}

void SoundSystemImpl::FreeChunk(Chunk_t* chunk) const { Mix_FreeChunk(chunk); }

const char* SoundSystemImpl::GetError() const { return Mix_GetError(); }

const uint16_t SoundSystemImpl::S8 = AUDIO_S8;
const uint16_t SoundSystemImpl::S16 = AUDIO_S16;
