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

#include "libsiglus/sgvm_factory.hpp"

#include "libsiglus/bindings/common.hpp"
#include "libsiglus/bindings/obj.hpp"
#include "libsiglus/bindings/sdl.hpp"
#include "libsiglus/bindings/system.hpp"

#include "m6/vm_factory.hpp"
#include "systems/sdl/sdl_system.hpp"

#include <filesystem>

namespace libsiglus {
namespace sr = serilang;
namespace fs = std::filesystem;

SiglusRuntime SGVMFactory::Create() {
  SiglusRuntime runtime;
  runtime.vm = std::make_unique<sr::VM>(m6::VMFactory::Create());

  Gameexe& gexe = runtime.gameexe;
  gexe.SetStringAt("CAPTION", "SiglusTest");
  gexe.SetStringAt("REGNAME", "sjis: SIGLUS\\TEST");
  gexe.SetIntAt("NAME_ENC", 0);
  gexe.SetIntAt("SUBTITLE", 0);
  gexe.SetIntAt("MOUSE_CURSOR", 0);
  gexe.SetStringAt("__GAMEPATH", (fs::temp_directory_path() / "game").string());
  gexe.parseLine("#SCREENSIZE_MOD=999.1920.1080");
  runtime.system = std::make_unique<SDLSystem>(gexe);

  binding::Context ctx;
  ctx.base_pth = fs::temp_directory_path() / "game";
  ctx.save_pth = ctx.base_pth / "save";
  ctx.asset_scanner = runtime.asset_scanner = runtime.system->GetAssetScanner();

  runtime.asset_scanner->IndexDirectory(ctx.base_pth);

  // add bindings here
  binding::SDL(ctx).Bind(runtime);
  binding::System(ctx).Bind(runtime);
  binding::Obj(ctx).Bind(runtime);

  return runtime;
}

}  // namespace libsiglus
