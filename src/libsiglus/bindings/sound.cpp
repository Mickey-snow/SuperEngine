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

#include "libsiglus/bindings/registry.hpp"
#include "srbind/srbind.hpp"
#include "systems/sound_system.hpp"
#include "systems/system.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <string>
#include <vector>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

class SiglusBgm {
 public:
  explicit SiglusBgm(System* sys) : system_(sys) {}

  void play(std::vector<sr::Value> args) { Play(std::move(args), true); }
  void play_oneshot(std::vector<sr::Value> args) {
    Play(std::move(args), false);
  }
  // TODO: Implement Wait
  void play_wait(std::vector<sr::Value> args) { Play(std::move(args), true); }

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
    state_ = fade_ms > 0 ? kFadeOut : kFree;
  }

  void pause(std::vector<sr::Value>) {
    if (system_)
      system_->sound().BgmPause();
    state_ = kPause;
  }

  void resume(std::vector<sr::Value>) {
    if (system_)
      system_->sound().BgmUnPause();
    state_ = kPlay;
  }
  // TODO: Implement Wait
  void resume_wait(std::vector<sr::Value> args) { resume(std::move(args)); }

  // TODO: Implement Wait
  void wait(std::vector<sr::Value>) {}
  int wait_key(std::vector<sr::Value>) { return 0; }
  void wait_fade(std::vector<sr::Value>) { state_ = kFree; }
  int wait_fade_key(std::vector<sr::Value>) {
    state_ = kFree;
    return 0;
  }

  int check(std::vector<sr::Value>) const {
    if (state_ == kFadeOut || state_ == kPause)
      return state_;
    if (system_ && system_->sound().BgmStatus())
      return kPlay;
    return state_ == kPlay ? kPlay : kFree;
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
    state_ = kPlay;
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

  static constexpr int kFree = 0;
  static constexpr int kPlay = 1;
  static constexpr int kFadeOut = 2;
  static constexpr int kPause = 3;

  System* system_;
  int state_ = kFree;
  int volume_ = 255;
  int volume_max_ = 255;
  int volume_min_ = 0;
  std::string registered_name_;
};

void BindSound(Context&, SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  auto bgm =
      m.bind_instance("bgm", std::make_unique<SiglusBgm>(runtime.system.get()));
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
}

RLVM_REGISTER(SiglusBindingRegistry, "sound", BindSound)

}  // namespace libsiglus::binding
