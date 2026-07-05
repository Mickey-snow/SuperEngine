// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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
#include "systems/text_system.hpp"
#include "vm/vm.hpp"

namespace libsiglus::binding {
namespace sb = srbind;
using namespace serilang;

void BindSyscom(SiglusRuntime& runtime) {
  auto* sys = runtime.system.get();

  VM& vm = *runtime.vm;
  sb::module_ m(vm, "syscom");
  m.def("menu_enable", [sys] { sys->EnableSyscom(); });
  m.def("menu_disable", [sys] { sys->DisableSyscom(); });

  m.def("get_new_qsave_no", [] {
    // TODO: implement save/load
    return 0;
  });
  m.def("qsave", [](int, int, int) {
    // TODO: implement save/load
    return 0;
  });

  m.def("set_enable_hidemwnd", [](bool) {
    // TODO
  });
  m.def("get_no_wipe_anime_onoff", [] {
    // TODO
    return 0;
  });
  m.def("set_automode", [sys](int val) -> void {
    if (!sys)
      return;
    sys->text().SetAutoMode(val);
  });
  m.def("set_all_volume_default", [sys]() -> void {
    if (!sys)
      return;
    auto& sound = sys->sound();
    sound.SetBgmVolumeMod(255);
    sound.SetKoeVolume(255, 0);
    sound.SetSeVolumeMod(255);
  });
  m.def("set_bgm_volume_default", [sys]() -> void {
    if (!sys)
      return;
    auto& sound = sys->sound();
    sound.SetBgmVolumeMod(255);
  });
  m.def("set_koe_volume_default", [sys]() -> void {
    if (!sys)
      return;
    auto& sound = sys->sound();
    sound.SetKoeVolume(255, 0);
  });
  m.def("set_koe_onoff_default", []() -> void {
    // TODO
  });
  m.def("set_bgm_onoff_default", [sys]() -> void {
    if (!sys)
      return;
    auto& sound = sys->sound();
    sound.SetBgmEnabled(1);
  });
  m.def("set_all_onoff_default", [] {
    // TODO
  });
  m.def("set_koemode_default", [] {
    // TODO
  });
  m.def("set_bgmfade_volume_default", [] {
    // TODO
  });
  m.def("set_bgmfade_onoff_default", [] {
    // TODO
  });
  m.def("set_pcm_volume_default", [] {
    // TODO
  });
  m.def("set_pcm_onoff_default", [] {
    // TODO
  });
  m.def("set_se_volume_default", [] {
    // TODO
  });
  m.def("set_se_onoff_default", [] {
    // TODO
  });
  m.def("set_mov_volume_default", [] {
    // TODO
  });
  m.def("set_mov_onoff_default", [] {
    // TODO
  });
  // TODO
  m.def("get_all_onoff", [] { return 1; });
  m.def("get_all_volume", [] { return 1; });
  m.def("get_bgm_onoff", [] { return 1; });
  m.def("get_bgm_volume", [] { return 1; });
  m.def("get_koe_onoff", [] { return 1; });
  m.def("get_koemode", [] { return 1; });
  m.def("get_koe_volume", [] { return 1; });
  m.def("get_bgmfade_onoff", [] { return 1; });
  m.def("get_bgmfade_volume", [] { return 1; });
  m.def("get_pcm_onoff", [] { return 1; });
  m.def("get_pcm_volume", [] { return 1; });
  m.def("get_se_onoff", [] { return 1; });
  m.def("get_se_volume", [] { return 1; });
  m.def("get_mov_onoff", [] { return 1; });
  m.def("get_mov_volume", [] { return 1; });
  m.def("get_all_onoff", [] { return 1; });
  m.def("get_bgm_onoff", [] { return 1; });
  m.def("get_koe_onoff", [] { return 1; });
  m.def("get_koemode", [] { return 1; });
  m.def("get_bgmfade_onoff", [] { return 1; });
  m.def("get_pcm_onoff", [] { return 1; });
  m.def("get_se_onoff", [] { return 1; });
  m.def("get_mov_onoff", [] { return 1; });
  m.def("get_mov_volume", [] { return 1; });
}

RLVM_REGISTER(SiglusBindingRegistry, "syscom", BindSyscom)

}  // namespace libsiglus::binding
