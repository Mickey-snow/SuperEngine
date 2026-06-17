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

#include "core/asset_scanner.hpp"
#include "core/gameexe.hpp"
#include "core/stage.hpp"
#include "libsiglus/archive.hpp"
#include "libsiglus/bindings/loader.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/gexedat.hpp"
#include "libsiglus/intern_name.hpp"
#include "libsiglus/siglus_scene_renderer.hpp"
#include "log/domain_logger.hpp"
#include "m6/vm_factory.hpp"
#include "srbind/module.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "utilities/file.hpp"
#include "utilities/mapped_file.hpp"
#include "vm/exception.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus {
namespace chr = std::chrono;
namespace fs = std::filesystem;
namespace sr = serilang;
namespace sb = srbind;

static DomainLogger logger("SiglusFactory");

namespace {

void SetIntVecIfMissing(Gameexe& gexe,
                        std::string_view key,
                        std::initializer_list<int> values) {
  if (gexe.Exists(key))
    return;

  std::vector<GexeVal> gexe_values;
  gexe_values.reserve(values.size());
  for (int value : values)
    gexe_values.emplace_back(value);
  gexe.SetAt(key, std::move(gexe_values));
}

void EnsureSiglusTextDefaults(Gameexe& gexe) {
  SetIntVecIfMissing(gexe, "WINDOW_ATTR", {255, 255, 255, 255, 0});
  SetIntVecIfMissing(gexe, "WINDOW.000.ATTR_MOD", {0});
  SetIntVecIfMissing(gexe, "WINDOW.000.ATTR", {255, 255, 255, 255, 0});
  SetIntVecIfMissing(gexe, "WINDOW.000.MOJI_SIZE", {25});
  SetIntVecIfMissing(gexe, "WINDOW.000.MOJI_CNT", {50, 3});
  SetIntVecIfMissing(gexe, "WINDOW.000.MOJI_REP", {0, 4});
  SetIntVecIfMissing(gexe, "WINDOW.000.LUBY_SIZE", {0});
  SetIntVecIfMissing(gexe, "WINDOW.000.MOJI_POS", {24, 24, 32, 32});
  SetIntVecIfMissing(gexe, "WINDOW.000.POS", {2, 96, 64});
  SetIntVecIfMissing(gexe, "WINDOW.000.INDENT_USE", {1});
  SetIntVecIfMissing(gexe, "WINDOW.000.NAME_MOD", {0});
  SetIntVecIfMissing(gexe, "WINDOW.000.KEYCUR_MOD", {0, 0, 0});
  SetIntVecIfMissing(gexe, "WINDOW.000.R_COMMAND_MOD", {0});
  SetIntVecIfMissing(gexe, "WINDOW.000.WAKU_SETNO", {0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.000", {255, 255, 255});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.254", {255, 255, 255});
}

}  // namespace

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
  rt.loader = std::make_unique<binding::Loader>(*rt.archive, vm, debug_);

  rt.base_pth = base_path_;
  rt.save_pth = rt.base_pth / "save";
  rt.asset_scanner = std::make_shared<AssetScanner>();
  rt.asset_scanner->IndexDirectory(rt.base_pth);

  rt.gameexe = std::make_shared<Gameexe>(LoadGameexe(rt.asset_scanner));
  Gameexe& gexe = *rt.gameexe;
  EnsureSiglusTextDefaults(gexe);
  gexe.SetStringAt("CAPTION", "SiglusTest");
  gexe.SetStringAt("REGNAME", "sjis: SIGLUS\\TEST");
  gexe.SetIntAt("NAME_ENC", 0);
  gexe.SetIntAt("SUBTITLE", 0);
  gexe.SetIntAt("MOUSE_CURSOR", 0);
  gexe.SetStringAt("__GAMEPATH", base_path_.string());
  gexe.parseLine("#SCREENSIZE_MOD=999,1920,1080");

  // Init sdl system
  rt.system = std::make_unique<System>(gexe, rt.asset_scanner);
  rt.stage =
      std::make_unique<Stage>(rt.system->graphics().GetObjectLayerSize());
  rt.renderer = std::make_shared<SiglusSceneRenderer>(*rt.stage, *rt.system);
  rt.system->graphics().BindSceneRenderer(rt.renderer);

  rt.local_config = std::make_shared<Gameexe>();
  rt.global_config = std::make_shared<Gameexe>();

  for (auto it = binding::SiglusBindingRegistry::cbegin();
       it != binding::SiglusBindingRegistry::cend(); ++it) {
    it->second(rt);
  }
  sb::module_ m(gc.get(), vm.globals_.get());

  m.def("__builtin_dbgprint",
        [](std::string str) { std::cerr << "[TRACE] " << str << std::endl; });
  m.def("__builtin_name", [](std::string str) {
    throw std::runtime_error("TODO: name() not implemented yet.");
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
  auto cb_holder = std::make_shared<std::function<void()>>();
  *cb_holder = [cb_holder, vm = rt.vm.get(), system = rt.system.get()]() {
    constexpr auto period =
        chr::duration_cast<chr::steady_clock::duration>(chr::seconds(1)) / 60;

    auto next = chr::steady_clock::now() + period;
    vm->scheduler_.PushCallbackAt(*cb_holder, next);
    system->Run();
  };
  rt.exec_sdl_callback = [cb_holder]() { (*cb_holder)(); };
  rt.vm->scheduler_.PushCallbackAfter(rt.exec_sdl_callback,
                                      chr::milliseconds(2));

  return rt;
}

}  // namespace libsiglus
