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

#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
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

  virtual void ExecuteSoundSystem() override;

  virtual rlSoundSettings const& GetSettings() const override;
  virtual void SetSettings(const rlSoundSettings& settings) override;

  virtual void SetUseKoeForCharacter(const int usekoe_id,
                                     const int enabled) override;
  virtual int ShouldUseKoeForCharacter(const int usekoe_id) const override;

  virtual void SetBgmEnabled(const int in) override;
  virtual void SetBgmVolumeMod(const int in) override;
  virtual void SetBgmVolumeScript(const int level, int fade_in_ms) override;

  virtual player_t GetBgm() const override;

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

  virtual void SetChannelVolume(const int channel, const int level) override;
  virtual void SetChannelVolume(const int channel,
                                const int level,
                                const int fade_time_in_ms) override;

  virtual int GetChannelVolume(const int channel) const override;

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

  virtual int is_se_enabled() const override;
  virtual void SetIsSeEnabled(const int in) override;

  virtual int se_volume_mod() const override;
  virtual void SetSeVolumeMod(const int in) override;

  virtual void PlaySe(const int se_num) override;

  virtual bool HasSe(const int se_num) override;

  virtual int GetKoeVolume() const override;
  virtual void SetKoeVolume(const int level, const int fadetime) override;

  virtual void KoePlay(int id) override;
  virtual void KoePlay(int id, int charid) override;

  virtual bool KoePlaying() const override;
  virtual void KoeStop() override;

  virtual void Reset() override;

  virtual System& system() override;

 private:
  virtual void KoePlayImpl(int id);

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

  // Computes the actual volume for a channel based on the per channel
  // and the per system volume.
  int compute_channel_volume(const int channel_volume,
                             const int system_volume) {
    return (channel_volume * system_volume) / 255;
  }

  // Stores data about an ongoing volume adjustment (such as those started by
  // fun wavSetVolume(int, int, int).)
  struct VolumeAdjustTask {
    VolumeAdjustTask(unsigned int current_time,
                     int in_start_volume,
                     int in_final_volume,
                     int fade_time_in_ms);

    unsigned int start_time;
    unsigned int end_time;

    int start_volume;
    int final_volume;

    // Calculate the volume for in_time
    int calculateVolumeFor(unsigned int in_time);
  };

  typedef std::map<int, VolumeAdjustTask> ChannelAdjustmentMap;

 private:
  System& system_;

  rlSoundSettings settings_;

  AudioTable audio_table_;

  // Per channel volume
  unsigned char channel_volume_[NUM_TOTAL_CHANNELS];

  // Open tasks that adjust the volume of a wave channel. We do this
  // because SDL_mixer doesn't provide this functionality and I'm
  // guessing other mixers don't either.
  ChannelAdjustmentMap pcm_adjustment_tasks_;

  std::unique_ptr<VolumeAdjustTask> bgm_adjustment_task_;

  // Maps each UseKoe id to one or more koePlay ids.
  std::multimap<int, int> usekoe_to_koeplay_mapping_;

  std::shared_ptr<IAssetScanner> voice_assets_;

  VoiceFactory voice_factory_;

  AVSpec sound_quality_;

  // The bridge to sdl sound implementor
  std::unique_ptr<ISoundSystem> sound_impl_;

  // boost::serialization support
  friend class boost::serialization::access;

  template <class Archive>
  void save(Archive& ar, const unsigned int file_version) const;

  template <class Archive>
  void load(Archive& ar, const unsigned int file_version);

  BOOST_SERIALIZATION_SPLIT_MEMBER();
};  // end of class SDLSoundSystem
