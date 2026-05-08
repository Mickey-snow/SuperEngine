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
#include "libsiglus/bindings/loader.hpp"
#include "libsiglus/bindings/registry.hpp"

#include "libsiglus/archive.hpp"
#include "libsiglus/gexedat.hpp"
#include "libsiglus/intern_name.hpp"
#include "libsiglus/siglus_runtime.hpp"
#include "log/domain_logger.hpp"
#include "m6/vm_factory.hpp"
#include "srbind/module.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_object.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "utilities/file.hpp"
#include "utilities/mapped_file.hpp"
#include "vm/exception.hpp"
#include "vm/function.hpp"
#include "vm/gc.hpp"

#include <chrono>
#include <filesystem>
#include <format>
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

inline void dbg_print(std::string str) {
  std::cerr << "[TRACE] " << str << std::endl;
}

SiglusRuntime SGVMFactory::Create() {
  SiglusRuntime rt;
  rt.vm = std::make_unique<sr::VM>(m6::VMFactory::Create());
  sr::VM& vm = *rt.vm;
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  fs::path seen_path = CorrectPathCase(base_path_ / "scene.pck");
  MappedFile archive_mf(seen_path);
  rt.archive = std::make_shared<Archive>(Archive::Create(archive_mf.Read()));
  rt.loader = std::make_unique<binding::Loader>(*rt.archive, vm);

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
  rt.system = std::make_unique<System>(gexe, rt.asset_scanner);

  for (auto it = binding::SiglusBindingRegistry::cbegin();
       it != binding::SiglusBindingRegistry::cend(); ++it) {
    it->second(ctx, rt);
  }
  sb::module_ m(gc.get(), vm.globals_.get());

  m.def("__builtin_dbgprint", dbg_print);
  m.def("__builtin_name", [](std::string str) {
    // not implemented yet
  });
  m.def("__builtin_textout", [](int kidoku, std::string text) {
    // not implemented yet
  });
  m.def("__builtin_load_scn",
        [loader = rt.loader.get()](int scnid) -> sr::Value {
          sr::Module* mod = loader->Load(scnid);
          return sr::Value(mod);
        });

  m.def("__builtin_farcall",
        [loader = rt.loader.get()](std::string scn, int zlabel) -> sr::Value {
          for (auto& c : scn)
            c = std::tolower(c);
          const std::string zname = GetZlabelId(zlabel);
          const std::string dbgname = std::format("SCENE{}@{}", scn, zlabel);
          sr::Module* mod = loader->Load(scn);

          if (!mod)
            throw sr::RuntimeError(
                std::format("Farcall {} could not load scene", dbgname));

          auto it = mod->globals->find(zname);
          if (it == mod->globals->cend())
            it = mod->globals->find("%%script");
          return it->second;
        });
  m.def("__builtin_usrcmd",
        [loader = rt.loader.get()](int scn, int entry,
                                   std::string name) -> sr::Value {
          const std::string cmdname = GetUsercmdId(entry);
          const std::string dbgname = std::format("{}:{}@{}", scn, entry, name);

          sr::Module* mod = loader->Load(scn);
          if (!mod) {
            throw sr::RuntimeError(std::format(
                "User command {} could not load scene {}", dbgname, scn));
          }

          auto it = mod->globals->find(cmdname);
          if (it == mod->globals->cend()) {
            std::string errmsg =
                std::format("User command {} does not exist in {}:{}", dbgname,
                            scn, mod->name);
            throw sr::RuntimeError(std::move(errmsg));
          }
          return it->second;
        });

  // abuse the vm scheduler to refresh sdl regularly
  std::function<void()>& cb = rt.exec_sdl_callback;
  cb = [&cb, &rt]() {
    constexpr auto period =
        chr::duration_cast<chr::steady_clock::duration>(chr::seconds(1)) / 60;

    auto next = chr::steady_clock::now() + period;
    rt.vm->scheduler_.PushCallbackAt(cb, next);

    // redraw
    GraphicsSystem& graphics = rt.system->graphics();
    for (auto& obj : graphics.GetForegroundObjects())
      obj.ExecuteMutators();
    for (auto& obj : graphics.GetBackgroundObjects())
      obj.ExecuteMutators();
    graphics.RenderFrame(true);

    // poll events
    rt.system->event().ExecuteEventSystem();
  };
  rt.vm->scheduler_.PushCallbackAfter(cb, chr::milliseconds(2));

  return rt;
}

}  // namespace libsiglus
