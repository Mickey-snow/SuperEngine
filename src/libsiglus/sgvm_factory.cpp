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
#include "libsiglus/bindings/event.hpp"
#include "libsiglus/bindings/gexe.hpp"
#include "libsiglus/bindings/mwnd.hpp"
#include "libsiglus/bindings/obj.hpp"
#include "libsiglus/bindings/proxy.hpp"
#include "libsiglus/bindings/sound.hpp"
#include "libsiglus/bindings/system.hpp"
#include "libsiglus/gexedat.hpp"

#include "libsiglus/archive.hpp"
#include "libsiglus/siglus_runtime.hpp"
#include "log/domain_logger.hpp"
#include "m6/vm_factory.hpp"
#include "srbind/module.hpp"
#include "srbind/srbind.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/event_system.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "utilities/file.hpp"
#include "utilities/mapped_file.hpp"
#include "vm/gc.hpp"
#include "vm/object.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace libsiglus {
namespace chr = std::chrono;
namespace fs = std::filesystem;
namespace sr = serilang;
namespace sb = srbind;

static DomainLogger logger("SiglusFactory");

// Load Gameexe.ini config
static Gameexe LoadGameexe(std::shared_ptr<AssetScanner> scanner) {
  auto pth = scanner->FindFile("Gameexe", {"dat", "ini"});
  if (!pth.has_value()) {
    logger(Severity::Error) << "Gameexe.dat not found: " << pth.error().what();
    return {};
  }

  if (pth->extension() == ".ini") {
    auto gexe = Gameexe::FromFile(pth.value());
    if (gexe.has_value())
      return gexe.value();
    logger(Severity::Error) << "Error while loading " << pth->string() << ": "
                            << gexe.error().message;
  } else {
    try {
      return CreateGexe(pth.value());
    } catch (std::exception& e) {
      logger(Severity::Error)
          << "Error while loading " << pth->string() << ": " << e.what();
    }
  }
  return {};
}

SiglusRuntime SGVMFactory::Create() {
  SiglusRuntime rt;
  rt.vm = std::make_unique<sr::VM>(m6::VMFactory::Create());
  sr::VM& vm = *rt.vm;
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  fs::path seen_path = CorrectPathCase(base_path_ / "scene.pck");
  MappedFile archive_mf(seen_path);
  rt.archive = std::make_shared<Archive>(Archive::Create(archive_mf.Read()));

  binding::Context ctx;
  ctx.base_pth = base_path_;
  ctx.save_pth = ctx.base_pth / "save";
  ctx.asset_scanner = rt.asset_scanner = std::make_shared<AssetScanner>();
  rt.asset_scanner->IndexDirectory(ctx.base_pth);

  rt.gameexe = std::make_shared<Gameexe>(LoadGameexe(rt.asset_scanner));
  Gameexe& gexe = *rt.gameexe;
  gexe.SetStringAt("CAPTION", "SiglusTest");
  gexe.SetStringAt("REGNAME", "sjis: SIGLUS\\TEST");
  gexe.SetIntAt("NAME_ENC", 0);
  gexe.SetIntAt("SUBTITLE", 0);
  gexe.SetIntAt("MOUSE_CURSOR", 0);
  gexe.SetStringAt("__GAMEPATH", base_path_.string());
  gexe.parseLine("#SCREENSIZE_MOD=999,1920,1080");

  // Init sdl system
  rt.system = std::make_unique<SDLSystem>(gexe, rt.asset_scanner);

  // add bindings here
  binding::Sound(ctx).Bind(rt);
  binding::System(ctx).Bind(rt);
  binding::Obj(ctx).Bind(rt);
  binding::sgEvent(ctx).Bind(rt);
  binding::sgGexe(ctx).Bind(rt);
  binding::MWND(ctx).Bind(rt);
  sb::module_ m(gc.get(), vm.globals_.get());
  rt.usrprop = std::make_unique<Usrprop_storage_t>();

  m.def("__builtin_dbgprint",
        [](std::string str) { std::cerr << "[TRACE] " << str << std::endl; });
  m.def("__builtin_name", [](std::string str) {
    // not implemented yet
  });
  m.def("__builtin_textout", [](int kidoku, std::string text) {
    // not implemented yet
  });
  m.def("__builtin_usrprop",
        [usrprop = rt.usrprop.get(), gc](int scene, int idx, std::string name) {
          long long hash = ((long long)scene << 32) | idx;
          return binding::MakeProxy(
              gc.get(),
              [usrprop, hash]() {
                auto it = usrprop->find(hash);
                return it == usrprop->cend() ? sr::nil : it->second;
              },
              [usrprop, hash](sr::Value val) {
                (*usrprop)[hash] = val;
                return sr::nil;
              });
        });
  m.def("__builtin_farcall", [](std::string scn, int zlabel) {
    // not implemented yet
  });

  // abuse the vm scheduler to refresh sdl regularly
  std::function<void()>& cb = rt.exec_sdl_callback;
  cb = [&cb, &rt]() {
    constexpr auto period =
        chr::duration_cast<chr::steady_clock::duration>(chr::seconds(1)) / 60;

    auto next = chr::steady_clock::now() + period;
    rt.vm->scheduler_.PushCallbackAt(cb, next);

    // redraw
    std::shared_ptr<GraphicsSystem> graphics = rt.system->graphics_system_;
    for (auto& obj : graphics->GetForegroundObjects())
      obj.ExecuteMutators();
    for (auto& obj : graphics->GetBackgroundObjects())
      obj.ExecuteMutators();
    graphics->RenderFrame(true);

    // poll events
    std::shared_ptr<EventSystem> event = rt.system->event_system_;
    event->ExecuteEventSystem();
  };
  rt.vm->scheduler_.PushCallbackAfter(cb, chr::milliseconds(2));

  return rt;
}

}  // namespace libsiglus
