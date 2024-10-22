// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2008 Elliot Glaysher
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

#ifndef SRC_SYSTEMS_SDL_SDL_SOUND_SYSTEM_H_
#define SRC_SYSTEMS_SDL_SDL_SOUND_SYSTEM_H_

#include <memory>
#include <string>

#include "lru_cache.hpp"
#include "systems/base/sound_system.h"
#include "systems/base/isound_system.h"

class SDLSoundSystem : public SoundSystem {
 public:
  explicit SDLSoundSystem(System& system,
                          std::unique_ptr<ISoundSystem> impl = nullptr);
  ~SDLSoundSystem();

  virtual void SetBgmEnabled(const int in) override;
  virtual void SetBgmVolumeMod(const int in) override;
  virtual void SetBgmVolumeScript(const int level, int fade_in_ms) override;
  virtual int BgmStatus() const override;
  virtual void BgmPlay(const std::string& bgm_name, bool loop) override;
  virtual void BgmPlay(const std::string& bgm_name,
                       bool loop,
                       int fade_in_ms) override;
  virtual void BgmPlay(const std::string& bgm_name,
                       bool loop,
                       int fade_in_ms,
                       int fade_out_ms) override;
  virtual void BgmStop() override;
  virtual void BgmPause() override;
  virtual void BgmUnPause() override;
  virtual void BgmFadeOut(int fade_out_ms) override;
  virtual std::string GetBgmName() const override;
  virtual bool BgmLooping() const override;
  virtual player_t GetBgm() const override;

  virtual void SetChannelVolume(const int channel, const int level) override;

  virtual void WavPlay(const std::string& wav_file, bool loop) override;
  virtual void WavPlay(const std::string& wav_file,
                       bool loop,
                       const int channel) override;
  virtual void WavPlay(const std::string& wav_file,
                       bool loop,
                       const int channel,
                       const int fadein_ms) override;
  virtual bool WavPlaying(const int channel) override;
  virtual void WavStop(const int channel) override;
  virtual void WavStopAll() override;
  virtual void WavFadeOut(const int channel, const int fadetime) override;

  virtual void PlaySe(const int se_num) override;

  virtual bool KoePlaying() const override;
  virtual void KoeStop() override;

  virtual void Reset() override;

 private:
  virtual void KoePlayImpl(int id) override;

  // Implementation to play a wave file. Two wavPlay() versions use this
  // underlying implementation, which is split out so the one that takes a raw
  // channel can verify its input.
  //
  // Both NUM_BASE_CHANNELS and NUM_EXTRA_WAVPLAY_CHANNELS are legal inputs for
  // |channel|.
  void WavPlayImpl(const std::string& wav_file, const int channel, bool loop);

  // Computes and passes a volume to SDL_mixer for |channel|.
  void SetChannelVolumeImpl(int channel);

  // Creates a player object from a name. Throws if the bgm isn't found.
  player_t LoadMusic(const std::string& bgm_name);

 private:
  AVSpec sound_quality_;

  // The bridge to sdl sound implementor
  std::unique_ptr<ISoundSystem> sound_impl_;

};  // end of class SDLSoundSystem

#endif  // SRC_SYSTEMS_SDL_SDL_SOUND_SYSTEM_H_
