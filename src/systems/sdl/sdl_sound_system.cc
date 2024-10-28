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

#include "systems/sdl/sdl_sound_system.h"

#include "base/avdec/audio_decoder.h"
#include "base/avspec.h"
#include "systems/base/event_system.h"
#include "systems/base/system.h"
#include "systems/base/system_error.h"
#include "systems/sdl/sound_implementor.h"
#include "utilities/exception.h"

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// Changes an incoming RealLive volume (0-256) to the range SDL_Mixer expects
// (0-128).
inline static int realLiveVolumeToSDLMixerVolume(int in_vol) {
  return in_vol / 2;
}

static const std::set<std::string> SOUNDFILETYPES{"wav", "ogg", "nwa", "mp3"};

// -----------------------------------------------------------------------
// RealLive Sound Qualities table
// -----------------------------------------------------------------------

static constexpr AVSpec s_reallive_sound_qualities[] = {
    (AVSpec){11025, AV_SAMPLE_FMT::S8, 2},   // 11 k_hz, 8 bit stereo
    (AVSpec){11025, AV_SAMPLE_FMT::S16, 2},  // 11 k_hz, 16 bit stereo
    (AVSpec){22050, AV_SAMPLE_FMT::S8, 2},   // 22 k_hz, 8 bit stereo
    (AVSpec){22050, AV_SAMPLE_FMT::S16, 2},  // 22 k_hz, 16 bit stereo
    (AVSpec){44100, AV_SAMPLE_FMT::S8, 2},   // 44 k_hz, 8 bit stereo
    (AVSpec){44100, AV_SAMPLE_FMT::S16, 2},  // 44 k_hz, 16 bit stereo
    (AVSpec){48000, AV_SAMPLE_FMT::S8, 2},   // 48 k_hz, 8 bit stereo
    (AVSpec){48000, AV_SAMPLE_FMT::S16, 2}   // 48 k_hz, 16 bit stereo
};

// -----------------------------------------------------------------------
// SDLSoundSystem::VolumeAdjustTask
// -----------------------------------------------------------------------
SDLSoundSystem::VolumeAdjustTask::VolumeAdjustTask(unsigned int current_time,
                                                   int in_start_volume,
                                                   int in_final_volume,
                                                   int fade_time_in_ms)
    : start_time(current_time),
      end_time(current_time + fade_time_in_ms),
      start_volume(in_start_volume),
      final_volume(in_final_volume) {}

int SDLSoundSystem::VolumeAdjustTask::calculateVolumeFor(unsigned int in_time) {
  int end_offset = end_time - start_time;
  int cur_offset = in_time - start_time;
  double percent = static_cast<double>(cur_offset) / end_offset;
  return start_volume +
         static_cast<int>(percent * (final_volume - start_volume));
}

// -----------------------------------------------------------------------
// SDLSoundSystem
// -----------------------------------------------------------------------
SDLSoundSystem::SDLSoundSystem(System& system,
                               std::unique_ptr<ISoundSystem> impl)
    : system_(system),
      settings_(system.gameexe()),
      audio_table_(system.gameexe()),
      voice_assets_(system.GetFileSystem()),
      voice_factory_(system.GetFileSystem()),
      sound_impl_(std::move(impl)) {
  if (sound_impl_ == nullptr)
    throw std::invalid_argument("Sound implementor is nullptr.");

  sound_impl_->InitSystem();

  /* We're going to be requesting certain things from our audio
     device, so we set them up beforehand */
  sound_quality_ = s_reallive_sound_qualities[settings_.sound_quality];
  static constexpr int audio_buffer_size = 4096;
  try {
    sound_impl_->OpenAudio(sound_quality_, audio_buffer_size);
    sound_impl_->AllocateChannels(NUM_TOTAL_CHANNELS);
  } catch (std::runtime_error& e) {
    std::ostringstream oss;
    oss << "Couldn't initialize audio: " << e.what();
    throw SystemError(oss.str());
  }

  Gameexe& gexe = system_.gameexe();

  std::fill_n(channel_volume_, NUM_TOTAL_CHANNELS, 255);

  // Read the \#KOEONOFF entries
  for (auto koeonoff : gexe.Filter("KOEONOFF.")) {
    std::vector<std::string> keyparts = koeonoff.GetKeyParts();
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

SDLSoundSystem::~SDLSoundSystem() {
  sound_impl_->CloseAudio();
  sound_impl_->QuitSystem();
}

void SDLSoundSystem::ExecuteSoundSystem() {
  unsigned int cur_time = system_.event().GetTicks();

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

rlSoundSettings const& SDLSoundSystem::GetSettings() const { return settings_; }

void SDLSoundSystem::SetSettings(const rlSoundSettings& settings) {
  settings_ = settings;
}

void SDLSoundSystem::SetUseKoeForCharacter(const int character,
                                           const int enabled) {
  auto range = usekoe_to_koeplay_mapping_.equal_range(character);
  for (auto it = range.first; it != range.second; ++it) {
    settings_.character_koe_enabled[it->second] = enabled;
  }
}

int SDLSoundSystem::ShouldUseKoeForCharacter(const int character) const {
  auto it = usekoe_to_koeplay_mapping_.find(character);
  if (it != usekoe_to_koeplay_mapping_.end()) {
    int koeplay_id = it->second;

    auto koe_it = settings_.character_koe_enabled.find(koeplay_id);
    if (koe_it != settings_.character_koe_enabled.end()) {
      return koe_it->second;
    }
  }

  // Default to true
  return 1;
}

void SDLSoundSystem::SetBgmEnabled(const int in) {
  if (in)
    sound_impl_->EnableBgm();
  else
    sound_impl_->DisableBgm();
  settings_.bgm_enabled = in;
}

void SDLSoundSystem::SetBgmVolumeMod(const int in) {
  settings_.bgm_volume = in;

  player_t player = sound_impl_->GetBgm();
  if (player) {
    player->SetVolume(static_cast<float>(in) / 255.0f);
  }
}

void SDLSoundSystem::SetBgmVolumeScript(const int level, int fade_in_ms) {
  if (fade_in_ms == 0) {
    settings_.bgm_volume = level;
  } else {
    unsigned int cur_time = system_.event().GetTicks();

    bgm_adjustment_task_.reset(new VolumeAdjustTask(
        cur_time, settings_.bgm_volume, level, fade_in_ms));
  }

  player_t player = sound_impl_->GetBgm();
  if (player) {
    float volume =
        static_cast<float>(settings_.bgm_volume * level) / (255.0f * 255.0f);
    player->SetVolume(volume);
  }
}

player_t SDLSoundSystem::GetBgm() const { return sound_impl_->GetBgm(); }

int SDLSoundSystem::BgmStatus() const {
  auto player = sound_impl_->GetBgm();
  if (player && player->GetStatus() == AudioPlayer::STATUS::PLAYING)
    return 1;
  return 0;
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name, bool loop) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = LoadMusic(bgm_name);
    player->SetLoopTimes(loop ? -1 : 0);
    sound_impl_->PlayBgm(player);
  }
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name,
                             bool loop,
                             int fade_in_ms) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = LoadMusic(bgm_name);
    player->FadeIn(fade_in_ms);
    player->SetLoopTimes(loop ? -1 : 0);
    sound_impl_->PlayBgm(player);
  }
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name,
                             bool loop,
                             int fade_in_ms,
                             int fade_out_ms) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = LoadMusic(bgm_name);
    player->FadeIn(fade_in_ms);
    player->SetLoopTimes(loop ? -1 : 0);
    // TODO: Fade out currently playing bgm
    sound_impl_->PlayBgm(player);
  }
}

void SDLSoundSystem::BgmStop() {
  player_t player = sound_impl_->GetBgm();
  if (player)
    player->Terminate();
}

void SDLSoundSystem::BgmPause() {
  player_t player = sound_impl_->GetBgm();
  if (player)
    player->Pause();
}

void SDLSoundSystem::BgmUnPause() {
  player_t player = sound_impl_->GetBgm();
  if (player)
    player->Unpause();
}

void SDLSoundSystem::BgmFadeOut(int fade_out_ms) {
  player_t player = sound_impl_->GetBgm();
  if (player)
    player->FadeOut(fade_out_ms);
}

std::string SDLSoundSystem::GetBgmName() const {
  auto player = sound_impl_->GetBgm();
  if (player)
    return player->GetName();
  return "";
}

bool SDLSoundSystem::BgmLooping() const {
  // TODO: maintain the looping status
  // this function is used for serialization
  return true;
}

void SDLSoundSystem::SetChannelVolume(const int channel, const int level) {
  channel_volume_[channel] = level;
  SetChannelVolumeImpl(channel);
}

void SDLSoundSystem::SetChannelVolume(const int channel,
                                      const int level,
                                      const int fade_time_in_ms) {
  unsigned int cur_time = system_.event().GetTicks();

  pcm_adjustment_tasks_.emplace(
      channel, VolumeAdjustTask(cur_time, channel_volume_[channel], level,
                                fade_time_in_ms));
}

int SDLSoundSystem::GetChannelVolume(const int channel) const {
  return channel_volume_[channel];
}

void SDLSoundSystem::WavPlay(const std::string& wav_file, bool loop) {
  int channel_number = -1;

  try {
    channel_number = sound_impl_->FindIdleChannel();
  } catch (std::runtime_error& e) {
    std::ostringstream oss;
    oss << "Couldn't find a free channel for wavPlay(): " << e.what();
    throw std::runtime_error(oss.str());
  }

  WavPlayImpl(wav_file, channel_number, loop);
}

void SDLSoundSystem::WavPlay(const std::string& wav_file,
                             bool loop,
                             const int channel) {
  WavPlayImpl(wav_file, channel, loop);
}

void SDLSoundSystem::WavPlay(const std::string& wav_file,
                             bool loop,
                             const int channel,
                             const int fadein_ms) {
  if (settings_.pcm_enabled) {
    auto wav_file_path = voice_assets_->FindFile(wav_file, SOUNDFILETYPES);
    player_t player = CreateAudioPlayer(wav_file_path);
    player->SetLoopTimes(loop ? -1 : 0);
    player->FadeIn(fadein_ms);
    SetChannelVolumeImpl(channel);
    sound_impl_->PlayChannel(channel, player);
  }
}

bool SDLSoundSystem::WavPlaying(const int channel) {
  return sound_impl_->IsPlaying(channel);
}

void SDLSoundSystem::WavStop(const int channel) {
  if (settings_.pcm_enabled) {
    sound_impl_->HaltChannel(channel);
  }
}

void SDLSoundSystem::WavStopAll() {
  if (settings_.pcm_enabled) {
    sound_impl_->HaltAllChannels();
  }
}

void SDLSoundSystem::WavFadeOut(const int channel, const int fadetime) {
  if (settings_.pcm_enabled)
    sound_impl_->FadeOutChannel(channel, fadetime);
}

int SDLSoundSystem::is_se_enabled() const { return settings_.se_enabled; }

void SDLSoundSystem::SetIsSeEnabled(const int in) { settings_.se_enabled = in; }

int SDLSoundSystem::se_volume_mod() const { return settings_.se_volume; }

void SDLSoundSystem::SetSeVolumeMod(const int level) {
  settings_.se_volume = level;
}

void SDLSoundSystem::PlaySe(const int se_num) {
  if (!is_se_enabled())
    return;

  auto se = audio_table_.FindSE(se_num);
  const std::string& file_name = se.file;
  int channel = se.channel;

  // Make sure there isn't anything playing on the current channel
  sound_impl_->HaltChannel(channel);
  if (file_name == "") {
    // Just stop a channel in case of an empty file name.
    return;
  }

  auto file_path = voice_assets_->FindFile(file_name, SOUNDFILETYPES);
  player_t player = CreateAudioPlayer(file_path);
  player->SetLoopTimes(0);

  // SE chunks have no volume other than the modifier.
  sound_impl_->SetVolume(channel,
                         realLiveVolumeToSDLMixerVolume(se_volume_mod()));
  sound_impl_->PlayChannel(channel, player);
}

bool SDLSoundSystem::HasSe(const int se_num) {
  try {
    auto se = audio_table_.FindSE(se_num);
  } catch (std::invalid_argument&) {
    return false;
  }
  return true;
}

void SDLSoundSystem::SetKoeVolume(const int level, const int fadetime) {
  if (fadetime == 0) {
    SetChannelVolume(KOE_CHANNEL, level);
  } else {
    SetChannelVolume(KOE_CHANNEL, level, fadetime);
  }
}

int SDLSoundSystem::GetKoeVolume() const {
  return GetChannelVolume(KOE_CHANNEL);
}

void SDLSoundSystem::KoePlay(int id) {
  if (!system_.ShouldFastForward())
    KoePlayImpl(id);
}

void SDLSoundSystem::KoePlay(int id, int charid) {
  if (!system_.ShouldFastForward()) {
    bool play_voice = true;

    auto koe_it = settings_.character_koe_enabled.find(charid);
    if (koe_it != settings_.character_koe_enabled.end()) {
      play_voice = koe_it->second;
    }

    if (play_voice) {
      KoePlayImpl(id);
    }
  }
}

void SDLSoundSystem::KoePlayImpl(int id) {
  if (!settings_.koe_enabled) {
    return;
  }

  VoiceClip voice_sample = voice_factory_.LoadSample(id);
  AudioDecoder decoder(voice_sample.content, voice_sample.format_name);
  auto player = std::make_shared<AudioPlayer>(std::move(decoder));

  SetChannelVolumeImpl(KOE_CHANNEL);
  sound_impl_->PlayChannel(KOE_CHANNEL, player);
}

bool SDLSoundSystem::KoePlaying() const {
  return sound_impl_->IsPlaying(KOE_CHANNEL);
}

void SDLSoundSystem::KoeStop() { sound_impl_->HaltChannel(KOE_CHANNEL); }

void SDLSoundSystem::Reset() {
  BgmStop();
  WavStopAll();
}

System& SDLSoundSystem::system() { return system_; }

void SDLSoundSystem::SetChannelVolumeImpl(int channel) {
  int base =
      channel == KOE_CHANNEL ? settings_.koe_volume : settings_.pcm_volume;
  int adjusted = compute_channel_volume(GetChannelVolume(channel), base);

  sound_impl_->SetVolume(channel, realLiveVolumeToSDLMixerVolume(adjusted));
}

player_t SDLSoundSystem::LoadMusic(const std::string& bgm_name) {
  auto track = audio_table_.FindBgm(bgm_name);
  auto file_path = voice_assets_->FindFile(track.file, SOUNDFILETYPES);

  player_t player = CreateAudioPlayer(file_path);
  player->SetName(track.name);
  player->SetPLoop(track.from, track.to, track.loop);
  return player;
}

void SDLSoundSystem::WavPlayImpl(const std::string& wav_file,
                                 const int channel,
                                 bool loop) {
  if (settings_.pcm_enabled) {
    fs::path wav_file_path = voice_assets_->FindFile(wav_file, SOUNDFILETYPES);
    player_t player = CreateAudioPlayer(wav_file_path);
    player->SetLoopTimes(loop ? -1 : 0);
    SetChannelVolumeImpl(channel);
    sound_impl_->PlayChannel(channel, player);
  }
}

template <class Archive>
void SDLSoundSystem::save(Archive& ar, unsigned int version) const {
  std::string track_name;
  bool looping = false;

  if (BgmStatus() == 1) {
    track_name = GetBgmName();
    looping = BgmLooping();
  }

  ar & track_name & looping;
}

template <class Archive>
void SDLSoundSystem::load(Archive& ar, unsigned int version) {
  std::string track_name;
  bool looping;
  ar & track_name & looping;

  if (track_name != "")
    BgmPlay(track_name, looping);
}

// Explicit instantiations for text archives (since we hide the implementation)
template void SDLSoundSystem::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void SDLSoundSystem::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
