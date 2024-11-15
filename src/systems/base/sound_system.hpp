// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#pragma once

#include <boost/serialization/serialization.hpp>
#include <map>
#include <string>
#include <utility>

#include "base/audio_player.hpp"
#include "base/audio_table.hpp"
#include "base/sound_settings.hpp"
#include "base/voice_factory.hpp"

class Gameexe;
class System;

constexpr int NUM_BASE_CHANNELS = 16;
constexpr int NUM_EXTRA_WAVPLAY_CHANNELS = 8;
constexpr int NUM_KOE_CHANNELS = 1;
constexpr int NUM_TOTAL_CHANNELS =
    NUM_BASE_CHANNELS + NUM_EXTRA_WAVPLAY_CHANNELS + NUM_KOE_CHANNELS;

// The koe channel is the last one.
constexpr int KOE_CHANNEL = NUM_BASE_CHANNELS + NUM_EXTRA_WAVPLAY_CHANNELS;

// -----------------------------------------------------------------------

class SoundSystem {
 public:
  virtual ~SoundSystem() = default;

  virtual void ExecuteSoundSystem() = 0;

  virtual rlSoundSettings const& GetSettings() const = 0;
  virtual void SetSettings(const rlSoundSettings& settings) = 0;

  virtual void SetUseKoeForCharacter(const int usekoe_id,
                                     const int enabled) = 0;
  virtual int ShouldUseKoeForCharacter(const int usekoe_id) const = 0;

  virtual void SetBgmEnabled(const int in) = 0;
  virtual void SetBgmVolumeMod(const int in) = 0;
  virtual void SetBgmVolumeScript(const int level, const int fade_in_ms) = 0;

  virtual player_t GetBgm() const = 0;

  virtual int BgmStatus() const = 0;

  virtual void BgmPlay(const std::string& bgm_name, bool loop) = 0;
  virtual void BgmPlay(const std::string& bgm_name,
                       bool loop,
                       int fade_in_ms) = 0;
  virtual void BgmPlay(const std::string& bgm_name,
                       bool loop,
                       int fade_in_ms,
                       int fade_out_ms) = 0;
  virtual void BgmStop() = 0;
  virtual void BgmPause() = 0;
  virtual void BgmUnPause() = 0;
  virtual void BgmFadeOut(int fade_out_ms) = 0;

  virtual std::string GetBgmName() const = 0;
  virtual bool BgmLooping() const = 0;

  virtual void SetChannelVolume(const int channel, const int level) = 0;
  virtual void SetChannelVolume(const int channel,
                                const int level,
                                const int fade_time_in_ms) = 0;

  virtual int GetChannelVolume(const int channel) const = 0;

  virtual void WavPlay(const std::string& wav_file, bool loop) = 0;
  virtual void WavPlay(const std::string& wav_file,
                       bool loop,
                       const int channel) = 0;
  virtual void WavPlay(const std::string& wav_file,
                       bool loop,
                       const int channel,
                       const int fadein_ms) = 0;
  virtual bool WavPlaying(const int channel) = 0;
  virtual void WavStop(const int channel) = 0;
  virtual void WavStopAll() = 0;
  virtual void WavFadeOut(const int channel, const int fadetime) = 0;

  virtual int is_se_enabled() const = 0;
  virtual void SetIsSeEnabled(const int in) = 0;

  virtual int se_volume_mod() const = 0;
  virtual void SetSeVolumeMod(const int in) = 0;

  virtual void PlaySe(const int se_num) = 0;

  virtual bool HasSe(const int se_num) = 0;

  virtual int GetKoeVolume() const = 0;
  virtual void SetKoeVolume(const int level, const int fadetime) = 0;

  virtual void KoePlay(int id) = 0;
  virtual void KoePlay(int id, int charid) = 0;

  virtual bool KoePlaying() const = 0;
  virtual void KoeStop() = 0;

  virtual void Reset() = 0;

  virtual System& system() = 0;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {}
};
