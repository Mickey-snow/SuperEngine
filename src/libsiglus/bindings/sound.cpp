// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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
// -----------------------------------------------------------------------

#include "libsiglus/bindings/bootstrap.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "srbind/srbind.hpp"
#include "systems/sound_system.hpp"
#include "systems/system.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

namespace {

class PlaybackState {
 public:
  static constexpr int kFree = 0;
  static constexpr int kPlay = 1;
  static constexpr int kFadeOut = 2;
  static constexpr int kPause = 3;

  void Set(int state) { state_ = state; }
  int state() const { return state_; }

  int Check(const std::function<bool()>& is_playing) const {
    if (state_ == kFadeOut || state_ == kPause)
      return state_;
    if (is_playing && is_playing())
      return kPlay;
    return state_ == kPlay ? kPlay : kFree;
  }

  sr::Value WaitForPlayback(sr::VM& vm,
                            System* system,
                            bool key_skip,
                            bool fade_only,
                            std::function<bool()> is_playing) {
    if (fade_only && state_ != kFadeOut)
      return MakeResolvedFuture(*vm.gc_);

    auto done = [this, is_playing = std::move(is_playing)] {
      if (!is_playing || !is_playing()) {
        if (state_ == kPlay || state_ == kFadeOut)
          state_ = kFree;
        return true;
      }
      return false;
    };
    return MakePollingWaitFuture(vm, std::move(done), key_skip,
                                 system ? system->event_ptr().get() : nullptr);
  }

 private:
  int state_ = kFree;
};

struct ExKoeCallParams {
  int koe = 0;
  int character = -1;
  bool wait = false;
  bool key_skip = false;

  static ExKoeCallParams ParseFrom(std::vector<sr::Value> raw_args,
                                   bool default_wait,
                                   bool default_key_skip) {
    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
    ExKoeCallParams params;
    params.wait = default_wait;
    params.key_skip = default_key_skip;

    if (!packet.args.empty())
      params.koe = AsInt(packet.args[0]).value_or(0);
    if (packet.args.size() > 1)
      params.character = AsInt(packet.args[1]).value_or(-1);

    ForEachKeywordId(packet.kwargs, [&](int id, const sr::Value& value) {
      switch (id) {
        case 0:
          params.koe = AsInt(value).value_or(params.koe);
          break;
        case 1:
          params.character = AsInt(value).value_or(params.character);
          break;
        case 2:
          params.wait = AsInt(value).value_or(params.wait ? 1 : 0) != 0;
          break;
        case 3:
          params.key_skip = AsInt(value).value_or(params.key_skip ? 1 : 0) != 0;
          break;
        case 4:
          // Legacy jitan speed has no voice-rate backend in rlvm yet.
          break;
        default:
          break;
      }
    });

    return params;
  }
};

}  // namespace

class SiglusGlobalKoe {
 public:
  explicit SiglusGlobalKoe(System* sys) : system_(sys) {}

  int exkoe(std::vector<sr::Value> args) {
    Play(ExKoeCallParams::ParseFrom(std::move(args), false, false));
    return 0;
  }

  sr::Value exkoe_play_wait(sr::VM& vm, std::vector<sr::Value> args) {
    auto params = ExKoeCallParams::ParseFrom(std::move(args), true, false);
    Play(params);
    return params.wait ? Wait(vm, params.key_skip)
                       : MakeResolvedFuture(*vm.gc_);
  }

  sr::Value exkoe_play_wait_key(sr::VM& vm, std::vector<sr::Value> args) {
    auto params = ExKoeCallParams::ParseFrom(std::move(args), true, true);
    Play(params);
    return params.wait ? Wait(vm, params.key_skip)
                       : MakeResolvedFuture(*vm.gc_);
  }

  void koe_stop(std::vector<sr::Value> args) {
    CallPacket packet = CallPacket::DecodeFrom(std::move(args));
    int fade_ms = 0;
    if (!packet.args.empty())
      fade_ms = AsInt(packet.args[0]).value_or(0);
    ForEachKeywordId(packet.kwargs, [&](int id, const sr::Value& value) {
      if (id == 0)
        fade_ms = AsInt(value).value_or(fade_ms);
    });

    if (!system_)
      return;

    if (fade_ms > 0)
      system_->sound().WavFadeOut(KOE_CHANNEL, fade_ms);
    else
      system_->sound().KoeStop();
  }

 private:
  void Play(const ExKoeCallParams& params) {
    if (!system_)
      return;

    system_->sound().KoeStop();
    if (params.character >= 0)
      system_->sound().KoePlay(params.koe, params.character);
    else
      system_->sound().KoePlay(params.koe);
  }

  sr::Value Wait(sr::VM& vm, bool key_skip) {
    auto done = [system = system_] {
      return !system || !system->sound().KoePlaying();
    };
    return MakePollingWaitFuture(
        vm, std::move(done), key_skip,
        system_ ? system_->event_ptr().get() : nullptr);
  }

  System* system_;
};

class SiglusBgm {
 public:
  explicit SiglusBgm(System* sys) : system_(sys) {}

  void play(std::vector<sr::Value> args) { Play(std::move(args), true); }
  void play_oneshot(std::vector<sr::Value> args) {
    Play(std::move(args), false);
  }
  sr::Value play_wait(sr::VM& vm, std::vector<sr::Value> args) {
    Play(std::move(args), false);
    return wait(vm, {});
  }

  void ready(std::vector<sr::Value> args) {
    if (!args.empty())
      registered_name_ = AsString(args[0]);
  }

  void stop(std::vector<sr::Value> args) {
    const int fade_ms = args.empty() ? 0 : AsInt(args[0]).value_or(0);
    if (system_) {
      if (fade_ms > 0)
        system_->sound().BgmFadeOut(fade_ms);
      else
        system_->sound().BgmStop();
    }
    playback_.Set(fade_ms > 0 ? PlaybackState::kFadeOut : PlaybackState::kFree);
  }

  void pause(std::vector<sr::Value>) {
    if (system_)
      system_->sound().BgmPause();
    playback_.Set(PlaybackState::kPause);
  }

  void resume(std::vector<sr::Value>) {
    if (system_)
      system_->sound().BgmUnPause();
    playback_.Set(PlaybackState::kPlay);
  }
  sr::Value resume_wait(sr::VM& vm, std::vector<sr::Value> args) {
    resume(std::move(args));
    return wait(vm, {});
  }

  sr::Value wait(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, false, false);
  }
  sr::Value wait_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, true, false);
  }
  sr::Value wait_fade(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, false, true);
  }
  sr::Value wait_fade_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, true, true);
  }

  int check(std::vector<sr::Value>) const {
    return playback_.Check(
        [this] { return system_ && system_->sound().BgmStatus(); });
  }

  void set_volume(std::vector<sr::Value> args) {
    if (args.empty())
      return;

    volume_ = AsInt(args[0]).value_or(volume_);
    const int fade_ms = args.size() > 1 ? AsInt(args[1]).value_or(0) : 0;
    if (system_)
      system_->sound().SetBgmVolumeScript(volume_, fade_ms);
  }

  void set_volume_max(std::vector<sr::Value> args) {
    if (!args.empty())
      volume_max_ = AsInt(args[0]).value_or(volume_max_);
  }

  void set_volume_min(std::vector<sr::Value> args) {
    if (!args.empty())
      volume_min_ = AsInt(args[0]).value_or(volume_min_);
  }

  int get_volume(std::vector<sr::Value>) const { return volume_; }

  std::string get_regist_name(std::vector<sr::Value>) const {
    if (!registered_name_.empty())
      return registered_name_;
    if (system_)
      return system_->sound().GetBgmName();
    return "";
  }

  int get_play_pos(std::vector<sr::Value>) const {
    if (!system_)
      return 0;
    auto player = system_->sound().GetBgm();
    return player ? static_cast<int>(player->GetCurrentTime()) : 0;
  }

 private:
  void Play(std::vector<sr::Value> args, bool loop) {
    if (args.empty())
      return;

    registered_name_ = AsString(args[0]);
    playback_.Set(PlaybackState::kPlay);
    if (!system_)
      return;

    if (args.size() > 2)
      system_->sound().BgmPlay(registered_name_, loop,
                               AsInt(args[1]).value_or(0),
                               AsInt(args[2]).value_or(0));
    else if (args.size() > 1)
      system_->sound().BgmPlay(registered_name_, loop,
                               AsInt(args[1]).value_or(0));
    else
      system_->sound().BgmPlay(registered_name_, loop);
  }

  sr::Value WaitForPlayback(sr::VM& vm, bool key_skip, bool fade_only) {
    return playback_.WaitForPlayback(vm, system_, key_skip, fade_only, [this] {
      return system_ && system_->sound().BgmStatus();
    });
  }

  System* system_;
  PlaybackState playback_;
  int volume_ = 255;
  int volume_max_ = 255;
  int volume_min_ = 0;
  std::string registered_name_;
};

class SiglusPcmch {
 public:
  SiglusPcmch(System* sys, int channel) : system_(sys), channel_(channel) {}

  void play(std::vector<sr::Value> args) {
    Play(std::move(args), false, false);
  }

  void play_loop(std::vector<sr::Value> args) {
    Play(std::move(args), true, false);
  }

  sr::Value play_wait(sr::VM& vm, std::vector<sr::Value> args) {
    Play(std::move(args), false, false);
    return wait(vm, {});
  }

  void ready(std::vector<sr::Value> args) {
    Play(std::move(args), false, true);
  }

  void ready_loop(std::vector<sr::Value> args) {
    Play(std::move(args), true, true);
  }

  void stop(std::vector<sr::Value> args) {
    const int fade_ms = args.empty() ? 0 : AsInt(args[0]).value_or(0);
    if (system_ && IsValidChannel()) {
      if (fade_ms > 0)
        system_->sound().WavFadeOut(channel_, fade_ms);
      else
        system_->sound().WavStop(channel_);
    }
    playback_.Set(fade_ms > 0 ? PlaybackState::kFadeOut : PlaybackState::kFree);
    loop_ = false;
  }

  void pause(std::vector<sr::Value>) { playback_.Set(PlaybackState::kPause); }

  void resume(std::vector<sr::Value> args) {
    const int fade_ms = args.empty() ? 0 : AsInt(args[0]).value_or(0);
    ready_ = false;
    if (!pcm_name_.empty())
      StartPlayback(fade_ms);
    playback_.Set(PlaybackState::kPlay);
  }

  sr::Value resume_wait(sr::VM& vm, std::vector<sr::Value> args) {
    resume(std::move(args));
    return wait(vm, {});
  }

  sr::Value wait(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, false, false);
  }
  sr::Value wait_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, true, false);
  }
  sr::Value wait_fade(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, false, true);
  }
  sr::Value wait_fade_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForPlayback(vm, true, true);
  }

  int check(std::vector<sr::Value>) const {
    return playback_.Check([this] {
      return system_ && IsValidChannel() &&
             system_->sound().WavPlaying(channel_);
    });
  }

  void set_volume(std::vector<sr::Value> args) {
    if (args.empty())
      return;

    const int fade_ms = args.size() > 1 ? AsInt(args[1]).value_or(0) : 0;
    SetVolume(AsInt(args[0]).value_or(volume_), fade_ms);
  }

  void set_vol_max(std::vector<sr::Value> args) {
    const int fade_ms = args.empty() ? 0 : AsInt(args[0]).value_or(0);
    SetVolume(kVolumeMax, fade_ms);
  }

  void set_vol_min(std::vector<sr::Value> args) {
    const int fade_ms = args.empty() ? 0 : AsInt(args[0]).value_or(0);
    SetVolume(kVolumeMin, fade_ms);
  }

  int get_volume(std::vector<sr::Value>) const { return volume_; }

 private:
  void Play(std::vector<sr::Value> args, bool loop, bool ready) {
    if (args.empty())
      return;

    pcm_name_ = AsString(args[0]);
    fade_in_ms_ = args.size() > 1 ? AsInt(args[1]).value_or(0) : 0;
    loop_ = loop;
    ready_ = ready;

    if (ready) {
      playback_.Set(PlaybackState::kFree);
      return;
    }

    StartPlayback(fade_in_ms_);
  }

  void StartPlayback(int fade_ms) {
    playback_.Set(PlaybackState::kPlay);
    if (system_ && IsValidChannel() && !pcm_name_.empty())
      system_->sound().WavPlay(pcm_name_, loop_, channel_, fade_ms);
  }

  void SetVolume(int volume, int fade_ms) {
    volume_ = volume;
    if (!system_ || !IsValidChannel())
      return;

    if (fade_ms > 0)
      system_->sound().SetChannelVolume(channel_, volume_, fade_ms);
    else
      system_->sound().SetChannelVolume(channel_, volume_);
  }

  bool IsValidChannel() const {
    return channel_ >= 0 && channel_ < NUM_TOTAL_CHANNELS;
  }

  sr::Value WaitForPlayback(sr::VM& vm, bool key_skip, bool fade_only) {
    return playback_.WaitForPlayback(vm, system_, key_skip, fade_only, [this] {
      return system_ && IsValidChannel() &&
             system_->sound().WavPlaying(channel_);
    });
  }

  static constexpr int kVolumeMin = 0;
  static constexpr int kVolumeMax = 255;

  System* system_;
  int channel_;
  PlaybackState playback_;
  int volume_ = kVolumeMax;
  int fade_in_ms_ = 0;
  bool loop_ = false;
  bool ready_ = false;
  std::string pcm_name_;
};

void BindSound(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  auto global_koe = std::make_shared<SiglusGlobalKoe>(runtime.system.get());
  m.def(
      "exkoe",
      [global_koe](std::vector<sr::Value> args) {
        return global_koe->exkoe(std::move(args));
      },
      sb::vararg);
  m.def(
      "exkoe_play_wait",
      [global_koe](sr::VM& vm, std::vector<sr::Value> args) -> sr::Value {
        return global_koe->exkoe_play_wait(vm, std::move(args));
      },
      sb::vararg);
  m.def(
      "exkoe_play_wait_key",
      [global_koe](sr::VM& vm, std::vector<sr::Value> args) -> sr::Value {
        return global_koe->exkoe_play_wait_key(vm, std::move(args));
      },
      sb::vararg);
  m.def(
      "koe_stop",
      [global_koe](std::vector<sr::Value> args) {
        global_koe->koe_stop(std::move(args));
      },
      sb::vararg);

  sb::class_<SiglusBgm> bgm_cls(m, "Bgm", false);
  auto bgm = bgm_cls.inst("bgm", runtime.system.get());
  bgm.def("play", &SiglusBgm::play, sb::vararg);
  bgm.def("play_oneshot", &SiglusBgm::play_oneshot, sb::vararg);
  bgm.def("play_wait", &SiglusBgm::play_wait, sb::vararg);
  bgm.def("ready", &SiglusBgm::ready, sb::vararg);
  bgm.def("stop", &SiglusBgm::stop, sb::vararg);
  bgm.def("pause", &SiglusBgm::pause, sb::vararg);
  bgm.def("resume", &SiglusBgm::resume, sb::vararg);
  bgm.def("resume_wait", &SiglusBgm::resume_wait, sb::vararg);
  bgm.def("wait", &SiglusBgm::wait, sb::vararg);
  bgm.def("wait_key", &SiglusBgm::wait_key, sb::vararg);
  bgm.def("wait_fade", &SiglusBgm::wait_fade, sb::vararg);
  bgm.def("wait_fade_key", &SiglusBgm::wait_fade_key, sb::vararg);
  bgm.def("check", &SiglusBgm::check, sb::vararg);
  bgm.def("set_volume", &SiglusBgm::set_volume, sb::vararg);
  bgm.def("set_volume_max", &SiglusBgm::set_volume_max, sb::vararg);
  bgm.def("set_volume_min", &SiglusBgm::set_volume_min, sb::vararg);
  bgm.def("get_volume", &SiglusBgm::get_volume, sb::vararg);
  bgm.def("get_regist_name", &SiglusBgm::get_regist_name, sb::vararg);
  bgm.def("get_play_pos", &SiglusBgm::get_play_pos, sb::vararg);

  sb::class_<SiglusPcmch> pcmch(m, "__SiglusPcmch");
  pcmch.def(sb::init([sys = runtime.system.get()](int channel) {
              return new SiglusPcmch(sys, channel);
            }),
            sb::arg("channel"));
  pcmch.def("play", &SiglusPcmch::play, sb::vararg);
  pcmch.def("play_loop", &SiglusPcmch::play_loop, sb::vararg);
  pcmch.def("play_wait", &SiglusPcmch::play_wait, sb::vararg);
  pcmch.def("ready", &SiglusPcmch::ready, sb::vararg);
  pcmch.def("ready_loop", &SiglusPcmch::ready_loop, sb::vararg);
  pcmch.def("stop", &SiglusPcmch::stop, sb::vararg);
  pcmch.def("pause", &SiglusPcmch::pause, sb::vararg);
  pcmch.def("resume", &SiglusPcmch::resume, sb::vararg);
  pcmch.def("resume_wait", &SiglusPcmch::resume_wait, sb::vararg);
  pcmch.def("wait", &SiglusPcmch::wait, sb::vararg);
  pcmch.def("wait_key", &SiglusPcmch::wait_key, sb::vararg);
  pcmch.def("wait_fade", &SiglusPcmch::wait_fade, sb::vararg);
  pcmch.def("wait_fade_key", &SiglusPcmch::wait_fade_key, sb::vararg);
  pcmch.def("check", &SiglusPcmch::check, sb::vararg);
  pcmch.def("set_volume", &SiglusPcmch::set_volume, sb::vararg);
  pcmch.def("set_vol_max", &SiglusPcmch::set_vol_max, sb::vararg);
  pcmch.def("set_vol_min", &SiglusPcmch::set_vol_min, sb::vararg);
  pcmch.def("get_volume", &SiglusPcmch::get_volume, sb::vararg);

  std::string src =
      std::format(kIndexedFactory, "__SiglusPcmchList", "__SiglusPcmch");
  src += "pcmch_list = __SiglusPcmchList();";
  Execute(vm, std::move(src));
}

RLVM_REGISTER(SiglusBindingRegistry, "sound", BindSound)

}  // namespace libsiglus::binding
