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

#ifndef SRC_SYSTEMS_BASE_SOUND_SYSTEM_H_
#define SRC_SYSTEMS_BASE_SOUND_SYSTEM_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <map>
#include <string>
#include <utility>

#include "base/audio_player.h"
#include "base/audio_table.h"
#include "base/sound_settings.h"
#include "base/voice_factory.h"

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



// -----------------------------------------------------------------------

// Generalized interface to sound commands.
class SoundSystem {
 protected:
  // Type for a parsed \#SE table.
  using SoundEffect = std::pair<std::string, int>;
  using SeTable = std::map<int, SoundEffect>;

  // Type for parsed \#DSTRACK entries.
  using DSTable = std::map<std::string, DSTrack>;

  // Type for parsed \#CDTRACK entries.
  using CDTable = std::map<std::string, CDTrack>;

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

 public:
  explicit SoundSystem(System& system);
  virtual ~SoundSystem();

  // Gives the sound system a chance to run; done once per game loop.
  //
  // Overriders MUST call SoundSystem::execute_sound_system because we rely on
  // it to handle volume adjustment tasks.
  virtual void ExecuteSoundSystem();

  // ---------------------------------------------------------------------

  rlSoundSettings const& GetSettings() const { return settings_; }
  void SetSettings(const rlSoundSettings& settings) { settings_ = settings; }

  // Sets whether we play voices for certain characters.
  void SetUseKoeForCharacter(const int usekoe_id, const int enabled);

  virtual void SetBgmEnabled(const int in) = 0;
  // Returns whether we should play voices for certain characters. This
  // function is tied to UseKoe() family of functions and should not be queried
  // from within rlvm; use the |globals_.character_koe_enabled| map instead.
  int ShouldUseKoeForCharacter(const int usekoe_id) const;

  virtual void SetBgmVolumeMod(const int in) = 0;
  // ---------------------------------------------------------------------

  virtual void SetBgmVolumeScript(const int level, const int fade_in_ms);
  virtual player_t GetBgm() const = 0;

  // Status of the music subsystem
  //
  // - 0 Idle
  // - 1 Playing music
  // - 2 Fading out music
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

  // ---------------------------------------------------------------------

  // @name PCM/Wave functions

  // Sets an individual channel volume
  virtual void SetChannelVolume(const int channel, const int level);

  // Change the volume smoothly; the change from the current volume to level
  // will take fade_time_in_ms
  void SetChannelVolume(const int channel,
                        const int level,
                        const int fade_time_in_ms);

  // Fetches an individual channel volume
  int GetChannelVolume(const int channel) const;

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

  // ---------------------------------------------------------------------

  // Sound Effect functions

  // Whether we should have interface sound effects
  int is_se_enabled() const { return settings_.se_enabled; }
  virtual void SetIsSeEnabled(const int in);

  // The volume of interface sound effects relative to other sound
  // playback. (0-255)
  int se_volume_mod() const { return settings_.se_volume; }
  virtual void SetSeVolumeMod(const int in);

  // Plays an interface sound effect. |se_num| is an index into the #SE table.
  virtual void PlaySe(const int se_num) = 0;

  // Returns whether there is a sound effect |se_num| in the table.
  virtual bool HasSe(const int se_num) {
    SeTable::const_iterator it = se_table().find(se_num);
    return it != se_table().end();
  }

  // ---------------------------------------------------------------------

  // Koe (voice) functions

  // The volume for all voice levels (0-255). If |fadetime| is non-zero,
  // the volume will change smoothly, with the change taking |fadetime| ms,
  // otherwise it will change instantly.
  int GetKoeVolume() const;
  virtual void SetKoeVolume(const int level, const int fadetime);

  void KoePlay(int id);
  void KoePlay(int id, int charid);

  virtual bool KoePlaying() const = 0;
  virtual void KoeStop() = 0;

  virtual void Reset();

  System& system() { return system_; }

 protected:
  SeTable& se_table() { return se_table_; }
  const DSTable& ds_table() { return ds_tracks_; }
  const CDTable& cd_table() { return cd_tracks_; }

  // Computes the actual volume for a channel based on the per channel
  // and the per system volume.
  int compute_channel_volume(const int channel_volume,
                             const int system_volume) {
    return (channel_volume * system_volume) / 255;
  }

  // Plays a voice sample.
  virtual void KoePlayImpl(int id) = 0;

  static void CheckChannel(int channel, const char* function_name);
  static void CheckVolume(int level, const char* function_name);

  std::shared_ptr<IAssetScanner> voice_assets_;

  VoiceFactory voice_factory_;

 private:
  System& system_;

 protected:
  rlSoundSettings settings_;

 private:
  // Defined music tracks (files)
  DSTable ds_tracks_;

  // Defined music tracks (cd tracks)
  CDTable cd_tracks_;

  // ---------------------------------------------------------------------

  // PCM/Wave sound effect data

  // Per channel volume
  unsigned char channel_volume_[NUM_TOTAL_CHANNELS];

  // Open tasks that adjust the volume of a wave channel. We do this
  // because SDL_mixer doesn't provide this functionality and I'm
  // guessing other mixers don't either.
  //
  // @note Depending on features in other systems, I may push this
  //       down to SDLSoundSystem later.
  ChannelAdjustmentMap pcm_adjustment_tasks_;

  std::unique_ptr<VolumeAdjustTask> bgm_adjustment_task_;

  // ---------------------------------------------------------------------

  // @name Interface sound effect data

  // Parsed #SE.index entries. Maps a sound effect number to the filename to
  // play and the channel to play it on.
  SeTable se_table_;

  // Maps each UseKoe id to one or more koePlay ids.
  std::multimap<int, int> usekoe_to_koeplay_mapping_;

  // boost::serialization support
  friend class boost::serialization::access;

  template <class Archive>
  void save(Archive& ar, const unsigned int file_version) const;

  template <class Archive>
  void load(Archive& ar, const unsigned int file_version);

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};  // end of class SoundSystem

#endif  // SRC_SYSTEMS_BASE_SOUND_SYSTEM_H_
