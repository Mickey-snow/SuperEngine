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
#include "vm/vm.hpp"

#include <string>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

class SoundHandle {
  System* system;

 public:
  SoundHandle(System* sys) : system(sys) {}

  void play(std::string name, bool loop) {
    system->sound().WavPlay(name, loop);
  }

  void bgm(std::string name, bool loop) { system->sound().BgmPlay(name, loop); }
};

void BindSound(Context&, SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<SoundHandle> sdl(m, "Sound");

  sdl.def(sb::init([sys = runtime.system.get()]() -> SoundHandle* {
    return new SoundHandle(sys);
  }));
  sdl.def("play", &SoundHandle::play, sb::arg("name"), sb::arg("loop") = false);
  sdl.def("bgm", &SoundHandle::bgm, sb::arg("name"), sb::arg("loop") = true);
}

RLVM_REGISTER(SiglusBindingRegistry, "sound", BindSound)

}  // namespace libsiglus::binding
