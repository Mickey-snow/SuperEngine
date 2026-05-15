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

#include "libsiglus/bindings/util.hpp"
#include "srbind/srbind.hpp"
#include "vm/vm.hpp"

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

void BindStage(Context&, SiglusRuntime& runtime) {
  auto& vm = *runtime.vm;

  std::string src = R"(
class LazyArray {
  fn __init__(self, klass){
    self.klass = klass;
    self.storage = [];
  }
  fn __getitem__(self, idx){
    while(self.storage.len() <= idx) self.storage.append(nil);
    if(self.storage[idx] == nil) self.storage[idx] = self.klass();
    return self.storage[idx];
  }
}

class Stage {
  fn __init__(self){
    self.object = nil;
    try{ self.object = LazyArray(Object); }
    catch(e){ print("Siglus stage.object binding unavailable:", e); }

    self.mwnd = nil;
    try{ self.mwnd = LazyArray(Mwnd); }
    catch(e){ print("Siglus stage.mwnd binding unavailable:", e); }

    self.objgroup = nil;
    try{ self.objgroup = LazyArray(Group); }
    catch(e){ print("Siglus stage.objgroup binding unavailable:", e); }

    self.btnsel = nil;
    try{ self.btnsel = LazyArray(Btnset); }
    catch(e){ print("Siglus stage.btnsel binding unavailable:", e); }

    self.world = nil;
    try{ self.world = LazyArray(World); }
    catch(e){ print("Siglus stage.world binding unavailable:", e); }

    self.effect = nil;
    try{ self.effect = LazyArray(Effect); }
    catch(e){ print("Siglus stage.effect binding unavailable:", e); }

    self.quake = nil;
    try{ self.quake = LazyArray(Quake); }
    catch(e){ print("Siglus stage.quake binding unavailable:", e); }
  }
}

stage = LazyArray(Stage);
stage_back = Stage();
stage_front = Stage();
stage_next = Stage();
)";
  Execute(vm, std::move(src));
  // TODO: Implement actual Mwnd, Group, Btnsel, World, Effect, Quake classes
}

RLVM_REGISTER(SiglusBindingRegistry, "1_stage", BindStage)

}  // namespace libsiglus::binding
