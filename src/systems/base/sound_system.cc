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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "systems/base/sound_system.h"

#include <boost/algorithm/string.hpp>

#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "libreallive/gameexe.h"
#include "machine/serialization.h"
#include "systems/base/event_system.h"
#include "systems/base/system.h"

// -----------------------------------------------------------------------
// SoundSystem::VolumeAdjustTask
// -----------------------------------------------------------------------
SoundSystem::VolumeAdjustTask::VolumeAdjustTask(unsigned int current_time,
                                                int in_start_volume,
                                                int in_final_volume,
                                                int fade_time_in_ms)
    : start_time(current_time),
      end_time(current_time + fade_time_in_ms),
      start_volume(in_start_volume),
      final_volume(in_final_volume) {}

int SoundSystem::VolumeAdjustTask::calculateVolumeFor(unsigned int in_time) {
  int end_offset = end_time - start_time;
  int cur_offset = end_time - in_time;
  double percent = 1 - (double(cur_offset) / end_offset);
  return start_volume + (percent * (final_volume - start_volume));
}

// -----------------------------------------------------------------------
// SoundSystem
// -----------------------------------------------------------------------
SoundSystem::SoundSystem(System& system)
    : voice_assets_(system.GetFileSystem()),
      voice_factory_(system.GetFileSystem()),
      system_(system),
      settings_(system.gameexe()) {
  Gameexe& gexe = system_.gameexe();

  std::fill_n(channel_volume_, NUM_TOTAL_CHANNELS, 255);

  // Read the \#SE.xxx entries from the Gameexe
  GameexeFilteringIterator se = gexe.FilterBegin("SE.");
  GameexeFilteringIterator end = gexe.FilterEnd();
  for (; se != end; ++se) {
    std::string raw_number = se->GetKeyParts().at(1);
    int entry_number = std::stoi(raw_number);

    std::string file_name = se->GetStringAt(0);
    int target_channel = se->GetIntAt(1);

    se_table_[entry_number] = std::make_pair(file_name, target_channel);
  }

  // Read the \#DSTRACK entries
  GameexeFilteringIterator dstrack = gexe.FilterBegin("DSTRACK");
  for (; dstrack != end; ++dstrack) {
    int from = dstrack->GetIntAt(0);
    int to = dstrack->GetIntAt(1);
    int loop = dstrack->GetIntAt(2);
    const std::string& file = dstrack->GetStringAt(3);
    std::string name = dstrack->GetStringAt(4);
    boost::to_lower(name);

    ds_tracks_[name] = DSTrack(name, file, from, to, loop);
  }

  // Read the \#CDTRACK entries
  GameexeFilteringIterator cdtrack = gexe.FilterBegin("CDTRACK");
  for (; cdtrack != end; ++cdtrack) {
    int from = cdtrack->GetIntAt(0);
    int to = cdtrack->GetIntAt(1);
    int loop = cdtrack->GetIntAt(2);
    std::string name = cdtrack->GetStringAt(3);
    boost::to_lower(name);

    cd_tracks_[name] = CDTrack(name, from, to, loop);
  }

  // Read the \#KOEONOFF entries
  GameexeFilteringIterator koeonoff = gexe.FilterBegin("KOEONOFF.");
  for (; koeonoff != end; ++koeonoff) {
    std::vector<std::string> keyparts = koeonoff->GetKeyParts();
    int usekoe_id = std::stoi(keyparts.at(1));

    // Find the corresponding koeplay ids.
    std::vector<int> koeplay_ids;
    const std::string& unprocessed_koeids = keyparts.at(2);
    if (unprocessed_koeids.find('(') != std::string::npos) {
      // We have a list that we need to parse out.
      std::string no_parens =
          unprocessed_koeids.substr(1, unprocessed_koeids.size() - 2);

      std::vector<std::string> string_koeplay_ids;
      boost::split(string_koeplay_ids, no_parens, boost::is_any_of(","));

      for (std::string const& string_id : string_koeplay_ids) {
        koeplay_ids.push_back(std::stoi(string_id));
      }
    } else {
      koeplay_ids.push_back(std::stoi(unprocessed_koeids));
    }

    int onoff = (keyparts.at(3) == "ON") ? 1 : 0;
    for (int id : koeplay_ids) {
      usekoe_to_koeplay_mapping_.emplace(usekoe_id, id);
      settings_.character_koe_enabled[id] = onoff;
    }
  }
}

SoundSystem::~SoundSystem() {}

void SoundSystem::ExecuteSoundSystem() {
  unsigned int cur_time = system().event().GetTicks();

  ChannelAdjustmentMap::iterator it = pcm_adjustment_tasks_.begin();
  while (it != pcm_adjustment_tasks_.end()) {
    if (cur_time >= it->second.end_time) {
      SetChannelVolume(it->first, it->second.final_volume);
      pcm_adjustment_tasks_.erase(it++);
    } else {
      int volume = it->second.calculateVolumeFor(cur_time);
      SetChannelVolume(it->first, volume);
      ++it;
    }
  }

  if (bgm_adjustment_task_) {
    if (cur_time >= bgm_adjustment_task_->end_time) {
      SetBgmVolumeScript(bgm_adjustment_task_->final_volume, 0);
      bgm_adjustment_task_.reset();
    } else {
      int volume = bgm_adjustment_task_->calculateVolumeFor(cur_time);
      SetBgmVolumeScript(volume, 0);
    }
  }
}

void SoundSystem::SetBgmVolumeScript(const int level, const int fade_in_ms) {
  CheckVolume(level, "SetBgmVolumeScript");

  if (fade_in_ms == 0) {
    settings_.bgm_volume = level;
  } else {
    unsigned int cur_time = system().event().GetTicks();

    bgm_adjustment_task_.reset(new VolumeAdjustTask(
        cur_time, settings_.bgm_volume, level, fade_in_ms));
  }
}

void SoundSystem::SetChannelVolume(const int channel, const int level) {
  CheckChannel(channel, "set_channel_volume");
  CheckVolume(level, "set_channel_volume");

  channel_volume_[channel] = level;
}

void SoundSystem::SetChannelVolume(const int channel,
                                   const int level,
                                   const int fade_time_in_ms) {
  CheckChannel(channel, "set_channel_volume");
  CheckVolume(level, "set_channel_volume");

  unsigned int cur_time = system().event().GetTicks();

  pcm_adjustment_tasks_.emplace(
      channel, VolumeAdjustTask(cur_time, channel_volume_[channel], level,
                                fade_time_in_ms));
}

int SoundSystem::GetChannelVolume(const int channel) const {
  CheckChannel(channel, "channel_volume");
  return channel_volume_[channel];
}

void SoundSystem::SetIsSeEnabled(const int in) { settings_.se_enabled = in; }

void SoundSystem::SetSeVolumeMod(const int level) {
  CheckVolume(level, "set_se_volume");
  settings_.se_volume = level;
}

void SoundSystem::SetUseKoeForCharacter(const int character,
                                        const int enabled) {
  auto range = usekoe_to_koeplay_mapping_.equal_range(character);
  std::multimap<int, int>::iterator it;
  for (it = range.first; it != range.second; ++it) {
    settings_.character_koe_enabled[it->second] = enabled;
  }
}

int SoundSystem::ShouldUseKoeForCharacter(const int character) const {
  std::multimap<int, int>::const_iterator it =
      usekoe_to_koeplay_mapping_.find(character);
  if (it != usekoe_to_koeplay_mapping_.end()) {
    // We can sample only the first id because they should all be equivalent.
    int koeplay_id = it->second;

    std::map<int, int>::const_iterator koe_it =
        settings_.character_koe_enabled.find(koeplay_id);
    if (koe_it != settings_.character_koe_enabled.end()) {
      return koe_it->second;
    }
  }

  // Default to true
  return 1;
}

void SoundSystem::SetKoeVolume(const int level, const int fadetime) {
  if (fadetime == 0) {
    SetChannelVolume(KOE_CHANNEL, level);
  } else {
    SetChannelVolume(KOE_CHANNEL, level, fadetime);
  }
}

int SoundSystem::GetKoeVolume() const { return GetChannelVolume(KOE_CHANNEL); }

void SoundSystem::KoePlay(int id) {
  if (!system_.ShouldFastForward())
    KoePlayImpl(id);
}

void SoundSystem::KoePlay(int id, int charid) {
  if (!system_.ShouldFastForward()) {
    bool play_voice = true;

    std::map<int, int>::const_iterator koe_it =
        settings_.character_koe_enabled.find(charid);
    if (koe_it != settings_.character_koe_enabled.end()) {
      play_voice = koe_it->second;
    }

    if (play_voice) {
      KoePlayImpl(id);
    }
  }
}

void SoundSystem::Reset() {
  // empty
}

// static
void SoundSystem::CheckChannel(int channel, const char* function_name) {
  if (channel < 0 || channel > NUM_TOTAL_CHANNELS) {
    std::ostringstream oss;
    oss << "Invalid channel number " << channel << " in " << function_name;
    throw std::runtime_error(oss.str());
  }
}

// static
void SoundSystem::CheckVolume(int level, const char* function_name) {
  if (level < 0 || level > 255) {
    std::ostringstream oss;
    oss << "Invalid volume \"" << level << "\" in " << function_name
        << ". Valid values are 0-255.";
    throw std::runtime_error(oss.str());
  }
}

template <class Archive>
void SoundSystem::load(Archive& ar, unsigned int version) {
  std::string track_name;
  bool looping;
  ar & track_name & looping;

  if (track_name != "")
    BgmPlay(track_name, looping);
}

template <class Archive>
void SoundSystem::save(Archive& ar, unsigned int version) const {
  std::string track_name;
  bool looping = false;

  if (BgmStatus() == 1) {
    track_name = GetBgmName();
    looping = BgmLooping();
  }

  ar & track_name & looping;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void SoundSystem::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void SoundSystem::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
