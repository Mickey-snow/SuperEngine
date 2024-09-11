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

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "base/avdec/audio_decoder.h"
#include "base/avspec.h"
#include "systems/base/system.h"
#include "systems/base/system_error.h"
#include "systems/base/voice_archive.h"
#include "utilities/exception.h"

namespace fs = std::filesystem;

// Changes an incoming RealLive volume (0-256) to the range SDL_Mixer expects
// (0-128).
inline static int realLiveVolumeToSDLMixerVolume(int in_vol) {
  return in_vol / 2;
}

// -----------------------------------------------------------------------
// RealLive Sound Qualities table
// -----------------------------------------------------------------------
struct RealLiveSoundQualities {
  int rate;
  AV_SAMPLE_FMT format;
};

// A mapping between SoundQualities() and the values need to be passed
// to Mix_OpenAudio()
static RealLiveSoundQualities s_real_live_sound_qualities[] = {
    {11025, AV_SAMPLE_FMT::S8},   // 11 k_hz, 8 bit stereo
    {11025, AV_SAMPLE_FMT::S16},  // 11 k_hz, 16 bit stereo
    {22050, AV_SAMPLE_FMT::S8},   // 22 k_hz, 8 bit stereo
    {22050, AV_SAMPLE_FMT::S16},  // 22 k_hz, 16 bit stereo
    {44100, AV_SAMPLE_FMT::S8},   // 44 k_hz, 8 bit stereo
    {44100, AV_SAMPLE_FMT::S16},  // 44 k_hz, 16 bit stereo
    {48000, AV_SAMPLE_FMT::S8},   // 48 k_hz, 8 bit stereo
    {48000, AV_SAMPLE_FMT::S16}   // 48 k_hz, 16 bit stereo
};

// -----------------------------------------------------------------------
// SDLSoundSystem (private)
// -----------------------------------------------------------------------
void SDLSoundSystem::WavPlayImpl(const std::string& wav_file,
                                 const int channel,
                                 bool loop) {
  if (is_pcm_enabled()) {
    fs::path wav_file_path = system().FindFile(wav_file, SOUND_FILETYPES);
    player_t player = CreateAudioPlayer(wav_file_path);
    player->SetLoopTimes(loop ? -1 : 0);
    SetChannelVolumeImpl(channel);
    sound_impl_->PlayChannel(channel, player);
  }
}

void SDLSoundSystem::SetChannelVolumeImpl(int channel) {
  int base = channel == KOE_CHANNEL ? GetKoeVolume_mod() : pcm_volume_mod();
  int adjusted = compute_channel_volume(GetChannelVolume(channel), base);
  sound_impl_->SetVolume(channel, realLiveVolumeToSDLMixerVolume(adjusted));
}

std::shared_ptr<SDLMusic> SDLSoundSystem::LoadMusic(
    const std::string& bgm_name) {
  DSTable::const_iterator ds_it =
      ds_table().find(boost::to_lower_copy(bgm_name));
  // if (ds_it != ds_table().end())
  //   return SDLMusic::CreateMusic(system(), ds_it->second);

  CDTable::const_iterator cd_it =
      cd_table().find(boost::to_lower_copy(bgm_name));
  if (cd_it != cd_table().end()) {
    std::ostringstream oss;
    oss << "CD music not supported yet. Could not play track \"" << bgm_name
        << "\"";
    throw std::runtime_error(oss.str());
  }

  std::ostringstream oss;
  oss << "Could not find music track \"" << bgm_name << "\"";
  throw std::runtime_error(oss.str());
}

player_t SDLSoundSystem::__LoadMusic(const std::string& bgm_name) {
  auto track = FindBgm(bgm_name);
  auto file_path = system().FindFile(track.file, SOUND_FILETYPES);

  player_t player = CreateAudioPlayer(file_path);
  player->SetName(track.name);
  player->SetPLoop(track.from, track.to, track.loop);
  return player;
}

SoundSystem::DSTrack SDLSoundSystem::FindBgm(const std::string& bgm_name) {
  DSTable::const_iterator ds_it =
      ds_table().find(boost::to_lower_copy(bgm_name));
  if (ds_it != ds_table().end())
    return ds_it->second;

  CDTable::const_iterator cd_it =
      cd_table().find(boost::to_lower_copy(bgm_name));
  if (cd_it != cd_table().end()) {
    std::ostringstream oss;
    oss << "CD music not supported yet. Could not play track \"" << bgm_name
        << "\"";
    throw std::runtime_error(oss.str());
  }

  std::ostringstream oss;
  oss << "Could not find music track \"" << bgm_name << "\"";
  throw std::runtime_error(oss.str());
}

// -----------------------------------------------------------------------
// SDLSoundSystem
// -----------------------------------------------------------------------
SDLSoundSystem::SDLSoundSystem(System& system,
                               std::shared_ptr<SoundSystemImpl> impl)
    : SoundSystem(system), sound_impl_(impl) {
  // Set up audio implementor for SDLMusic and SDLSoundChunk
  // if an implementor is not specified, use default sdl
  if (sound_impl_ == nullptr)
    sound_impl_ = std::make_shared<SoundSystemImpl>();
  sound_impl_->InitSystem();

  /* We're going to be requesting certain things from our audio
     device, so we set them up beforehand */
  int audio_rate = s_real_live_sound_qualities[sound_quality()].rate;
  auto audio_format = s_real_live_sound_qualities[sound_quality()].format;
  int audio_channels = 2;
  int audio_buffers = 4096;

  /* This is where we open up our audio device.  Mix_OpenAudio takes
     as its parameters the audio format we'd /like/ to have. */

  if (sound_impl_->OpenAudio(audio_rate, audio_format, audio_channels,
                             audio_buffers)) {
    std::ostringstream oss;
    oss << "Couldn't initialize audio: " << sound_impl_->GetError();
    throw SystemError(oss.str());
  }

  sound_impl_->AllocateChannels(NUM_TOTAL_CHANNELS);
  sound_impl_->HookMusic(&SoundSystemImpl::OnMusic, NULL);
}

SDLSoundSystem::~SDLSoundSystem() {
  sound_impl_->HookMusic(NULL, NULL);
  sound_impl_->CloseAudio();
  sound_impl_->QuitSystem();
}

void SDLSoundSystem::ExecuteSoundSystem() {
  SoundSystem::ExecuteSoundSystem();

  // if (queued_music_ && !SDLMusic::IsCurrentlyPlaying()) {
  //   queued_music_->FadeIn(queued_music_loop_, queued_music_fadein_);
  //   queued_music_.reset();
  // }
}

void SDLSoundSystem::SetBgmEnabled(const int in) {
  if (in)
    sound_impl_->EnableBgm();
  else
    sound_impl_->DisableBgm();
  SoundSystem::SetBgmEnabled(in);
}

void SDLSoundSystem::SetBgmVolumeMod(const int in) {
  SoundSystem::SetBgmVolumeMod(in);
  // SDLMusic::SetComputedBgmVolume(
  //     compute_channel_volume(in, bgm_volume_script()));

  player_t player = sound_impl_->GetBgm();
  if (player) {
    float volume =
        static_cast<float>(in * bgm_volume_script()) / (255.0 * 255.0);
    player->SetVolume(volume);
  }
}

void SDLSoundSystem::SetBgmVolumeScript(const int level, int fade_in_ms) {
  SoundSystem::SetBgmVolumeScript(level, fade_in_ms);
  if (fade_in_ms == 0) {
    // If a fade was requested by the script, we don't want to set the volume
    // here right now. This is only slightly cleaner than having separate
    // methods because of the function casting in the modules.
    // SDLMusic::SetComputedBgmVolume(
    //     compute_channel_volume(bgm_volume_mod(), level));

    player_t player = sound_impl_->GetBgm();
    if (player) {
      float volume =
          static_cast<float>(bgm_volume_mod() * level) / (255.0 * 255.0);
      player->SetVolume(volume);
    }
  }
}

void SDLSoundSystem::SetChannelVolume(const int channel, const int level) {
  SoundSystem::SetChannelVolume(channel, level);
  SetChannelVolumeImpl(channel);
}

void SDLSoundSystem::WavPlay(const std::string& wav_file, bool loop) {
  int channel_number = sound_impl_->FindIdleChannel();
  if (channel_number == -1) {
    std::ostringstream oss;
    oss << "Couldn't find a free channel for wavPlay()";
    throw std::runtime_error(oss.str());
  }

  WavPlayImpl(wav_file, channel_number, loop);
}

void SDLSoundSystem::WavPlay(const std::string& wav_file,
                             bool loop,
                             const int channel) {
  CheckChannel(channel, "SDLSoundSystem::wav_play");
  WavPlayImpl(wav_file, channel, loop);
}

void SDLSoundSystem::WavPlay(const std::string& wav_file,
                             bool loop,
                             const int channel,
                             const int fadein_ms) {
  CheckChannel(channel, "SDLSoundSystem::wav_play");

  if (is_pcm_enabled()) {
    auto wav_file_path = system().FindFile(wav_file, SOUND_FILETYPES);
    player_t player = CreateAudioPlayer(wav_file_path);
    player->SetLoopTimes(loop ? -1 : 0);
    player->FadeIn(fadein_ms);
    SetChannelVolumeImpl(channel);
    sound_impl_->PlayChannel(channel, player);
  }
}

bool SDLSoundSystem::WavPlaying(const int channel) {
  CheckChannel(channel, "SDLSoundSystem::wav_playing");
  return sound_impl_->IsPlaying(channel);
}

void SDLSoundSystem::WavStop(const int channel) {
  CheckChannel(channel, "SDLSoundSystem::wav_stop");

  if (is_pcm_enabled()) {
    sound_impl_->HaltChannel(channel);
  }
}

void SDLSoundSystem::WavStopAll() {
  if (is_pcm_enabled()) {
    sound_impl_->HaltAllChannels();
  }
}

void SDLSoundSystem::WavFadeOut(const int channel, const int fadetime) {
  CheckChannel(channel, "SDLSoundSystem::wav_fade_out");

  if (is_pcm_enabled())
    sound_impl_->FadeOutChannel(channel, fadetime);
}

void SDLSoundSystem::PlaySe(const int se_num) {
  if (is_se_enabled()) {
    SeTable::const_iterator it = se_table().find(se_num);
    if (it == se_table().end()) {
      std::ostringstream oss;
      oss << "No #SE entry found for sound effect number " << se_num;
      throw rlvm::Exception(oss.str());
    }

    const std::string& file_name = it->second.first;
    int channel = it->second.second;

    // Make sure there isn't anything playing on the current channel
    sound_impl_->HaltChannel(channel);
    if (file_name == "") {
      // Just stop a channel in case of an empty file name.
      return;
    }

    auto file_path = system().FindFile(file_name, SOUND_FILETYPES);
    player_t player = CreateAudioPlayer(file_path);
    player->SetLoopTimes(0);

    // SE chunks have no volume other than the modifier.
    sound_impl_->SetVolume(channel,
                           realLiveVolumeToSDLMixerVolume(se_volume_mod()));
    sound_impl_->PlayChannel(channel, player);
  }
}

int SDLSoundSystem::BgmStatus() const {
  auto player = sound_impl_->GetBgm();
  if (!player)
    return 0;

  if (player->GetStatus() == AudioPlayer::STATUS::PLAYING)
    return 1;
  return 0;
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name, bool loop) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = __LoadMusic(bgm_name);
    sound_impl_->PlayBgm(player);
  }
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name,
                             bool loop,
                             int fade_in_ms) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = __LoadMusic(bgm_name);
    player->FadeIn(fade_in_ms);
    sound_impl_->PlayBgm(player);
  }
}

void SDLSoundSystem::BgmPlay(const std::string& bgm_name,
                             bool loop,
                             int fade_in_ms,
                             int fade_out_ms) {
  if (!boost::iequals(GetBgmName(), bgm_name)) {
    player_t player = __LoadMusic(bgm_name);
    player->FadeIn(fade_in_ms);
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

bool SDLSoundSystem::BgmLooping() const { return true; }

bool SDLSoundSystem::KoePlaying() const {
  return sound_impl_->IsPlaying(KOE_CHANNEL);
}

void SDLSoundSystem::KoeStop() { sound_impl_->HaltChannel(KOE_CHANNEL); }

void SDLSoundSystem::KoePlayImpl(int id) {
  if (!is_koe_enabled()) {
    return;
  }

  VoiceClip voice_sample = voice_factory_.LoadSample(id);
  AudioDecoder decoder(voice_sample.content, voice_sample.format_name);
  auto player = std::make_shared<AudioPlayer>(std::move(decoder));

  SetChannelVolumeImpl(KOE_CHANNEL);
  sound_impl_->PlayChannel(KOE_CHANNEL, player);
}

void SDLSoundSystem::Reset() {
  BgmStop();
  WavStopAll();

  SoundSystem::Reset();
}
