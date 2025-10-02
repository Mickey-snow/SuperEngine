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
#include "libsiglus/bindings/sound.hpp"
#include "libsiglus/bindings/system.hpp"
#include "libsiglus/gexedat.hpp"

#include "log/domain_logger.hpp"
#include "m6/vm_factory.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/event_system.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_system.hpp"

#include <chrono>
#include <filesystem>

namespace libsiglus {
namespace chr = std::chrono;
namespace sr = serilang;
namespace fs = std::filesystem;

static DomainLogger logger("SiglusFactory");

SiglusRuntime SGVMFactory::Create() {
  SiglusRuntime runtime;
  runtime.vm = std::make_unique<sr::VM>(m6::VMFactory::Create());

  binding::Context ctx;
  ctx.base_pth = fs::temp_directory_path() / "game";
  ctx.save_pth = ctx.base_pth / "save";
  ctx.asset_scanner = runtime.asset_scanner = std::make_shared<AssetScanner>();
  runtime.asset_scanner->IndexDirectory(ctx.base_pth);

  // Load Gameexe.ini config
  if (auto pth = ctx.asset_scanner->FindFile("Gameexe", {"dat"});
      pth.has_value())
    runtime.gameexe = CreateGexe(pth.value());
  else
    logger(Severity::Error) << "Gameexe.dat not found: " << pth.error().what();
  Gameexe& gexe = runtime.gameexe;
  gexe.SetStringAt("CAPTION", "SiglusTest");
  gexe.SetStringAt("REGNAME", "sjis: SIGLUS\\TEST");
  gexe.SetIntAt("NAME_ENC", 0);
  gexe.SetIntAt("SUBTITLE", 0);
  gexe.SetIntAt("MOUSE_CURSOR", 0);
  gexe.SetStringAt("__GAMEPATH", (fs::temp_directory_path() / "game").string());
  gexe.parseLine("#SCREENSIZE_MOD=999,1920,1080");

  // Init sdl system
  runtime.system = std::make_unique<SDLSystem>(gexe, runtime.asset_scanner);

  // add bindings here
  binding::Sound(ctx).Bind(runtime);
  binding::System(ctx).Bind(runtime);
  binding::Obj(ctx).Bind(runtime);

  // abuse the vm scheduler to refresh sdl regularly
  std::function<void()>& cb = runtime.exec_sdl_callback;
  cb = [&cb, &runtime]() {
    constexpr auto period =
        chr::duration_cast<chr::steady_clock::duration>(chr::seconds(1)) / 60;

    auto next = chr::steady_clock::now() + period;
    runtime.vm->scheduler_.PushDaemonAt(cb, next);

    // redraw
    std::shared_ptr<SDLGraphicsSystem> graphics =
        runtime.system->graphics_system_;
    for (auto& obj : graphics->GetForegroundObjects())
      obj.ExecuteMutators();
    for (auto& obj : graphics->GetBackgroundObjects())
      obj.ExecuteMutators();
    graphics->RenderFrame(true);

    // poll events
    std::shared_ptr<EventSystem> event = runtime.system->event_system_;
    event->ExecuteEventSystem();
  };
  runtime.vm->scheduler_.PushDaemonAfter(cb, chr::milliseconds(2));

  return runtime;
}

}  // namespace libsiglus
