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

#ifndef SRC_BASE_SOUND_SETTINGS_H_
#define SRC_BASE_SOUND_SETTINGS_H_

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>

#include <map>

class Gameexe;

// Global sound settings and data, saved and restored when rlvm is shutdown and
// started up.
struct rlSoundSettings {
  rlSoundSettings();
  explicit rlSoundSettings(Gameexe& gexe);

  // Number passed in from RealLive that represents what we want the
  // sound system to do. Right now is fairly securely set to 5 since I
  // have no idea how to change this property at runtime.
  //
  // 0              11 k_hz               8 bit
  // 1              11 k_hz               16 bit
  // 2              22 k_hz               8 bit
  // 3              22 k_hz               16 bit
  // 4              44 k_hz               8 bit
  // 5              44 k_hz               16 bit
  // 6              48 k_hz               8 bit
  // 7              48 h_kz               16 bit
  int sound_quality;

  // Whether music playback is enabled
  bool bgm_enabled;

  // Volume for the music
  int bgm_volume_mod;

  // Whether the Wav functions are enabled
  bool pcm_enabled;

  // Volume of wave files relative to other sound playback.
  int pcm_volume_mod;

  // Whether the Se functions are enabled
  bool se_enabled;

  // Volume of interface sound effects relative to other sound playback.
  int se_volume_mod;

  // Voice playback mode (see setKoeMode() for details).
  int koe_mode;

  // Whether we play any voiceovers.
  bool koe_enabled;

  // Volume of the koe relative to other sound playback.
  int GetKoeVolume_mod;

  // Whether we fade the background music when a voiceover is playing.
  bool bgm_koe_fade;

  // How much to modify the bgm volume if |bgm_koe_fade| is on.
  int bgm_koe_fade_vol;

  // Maps between a koePlay character number, and whether we enable voices for
  // them.
  std::map<int, int> character_koe_enabled;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & sound_quality & bgm_enabled & bgm_volume_mod & pcm_enabled &
        pcm_volume_mod & se_enabled & se_volume_mod;

    if (version >= 1) {
      ar & koe_mode & koe_enabled & GetKoeVolume_mod & bgm_koe_fade &
          bgm_koe_fade_vol & character_koe_enabled;
    }
  }
};

BOOST_CLASS_VERSION(rlSoundSettings, 1);

using SoundSystemGlobals = rlSoundSettings;

#endif
