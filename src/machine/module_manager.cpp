// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
// Copyright (C) 2006 Elliot Glaysher
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
//
// -----------------------------------------------------------------------

#include "module_manager.hpp"

#include "libreallive/elements/command.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"

#include "modules/module_bgm.hpp"
#include "modules/module_bgr.hpp"
#include "modules/module_debug.hpp"
#include "modules/module_dll.hpp"
#include "modules/module_event_loop.hpp"
#include "modules/module_g00.hpp"
#include "modules/module_gan.hpp"
#include "modules/module_grp.hpp"
#include "modules/module_jmp.hpp"
#include "modules/module_koe.hpp"
#include "modules/module_mem.hpp"
#include "modules/module_mov.hpp"
#include "modules/module_msg.hpp"
#include "modules/module_obj_creation.hpp"
#include "modules/module_obj_fg_bg.hpp"
#include "modules/module_obj_getters.hpp"
#include "modules/module_obj_management.hpp"
#include "modules/module_os.hpp"
#include "modules/module_pcm.hpp"
#include "modules/module_refresh.hpp"
#include "modules/module_scr.hpp"
#include "modules/module_se.hpp"
#include "modules/module_sel.hpp"
#include "modules/module_shk.hpp"
#include "modules/module_shl.hpp"
#include "modules/module_str.hpp"
#include "modules/module_sys.hpp"

#include <stdexcept>
#include <utility>

void ModuleManager::AttachModule(std::unique_ptr<RLModule> mod) {
  if (!mod)
    return;

  const auto hash = std::make_pair(mod->module_type(), mod->module_number());
  auto result = Get().modules_.try_emplace(hash, std::move(mod));
  if (!result.second) {
    throw std::invalid_argument(
        "Module hash clash: " + std::to_string(std::get<0>(hash)) + ',' +
        std::to_string(std::get<1>(hash)));
  }
}

void ModuleManager::DetachAll() { Get() = Ctx(); }

IModuleManager& ModuleManager::GetInstance() {
  class ModuleManager_impl : public IModuleManager {
   public:
    ModuleManager_impl(Ctx& ctx_in) : ctx(ctx_in) {}

    RLModule* GetModule(int module_type, int module_id) const {
      auto it = ctx.modules_.find(std::make_pair(module_type, module_id));
      if (it != ctx.modules_.cend())
        return it->second.get();
      return nullptr;
    }

    RLOperation* Dispatch(const libreallive::CommandElement& cmd) const {
      auto mod = GetModule(cmd.modtype(), cmd.module());
      if (mod == nullptr)
        return nullptr;

      return mod->Dispatch(cmd);
    }

    std::string GetCommandName(const libreallive::CommandElement& f) const {
      auto op = Dispatch(f);
      if (op == nullptr)
        return {};
      return op->Name();
    }

   private:
    Ctx& ctx;
  };

  static ModuleManager_impl instance(Get());
  return instance;
}

// -----------------------------------------------------------------------

ModuleManager::Ctx& ModuleManager::Get() {
  static Ctx ctx_;
  return ctx_;
}

// Add all modules by default

namespace {
[[maybe_unused]] bool __attach_all_modules_result = []() {
  try {
    ModuleManager::AttachModule(std::make_unique<BgmModule>());
    ModuleManager::AttachModule(std::make_unique<BgrModule>());
    ModuleManager::AttachModule(std::make_unique<BraModule>());
    ModuleManager::AttachModule(std::make_unique<ChildGanBgModule>());
    ModuleManager::AttachModule(std::make_unique<ChildGanFgModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjBgCreationModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjBgGettersModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjBgManagement>());
    ModuleManager::AttachModule(std::make_unique<ChildObjBgModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjFgCreationModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjFgGettersModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjFgManagement>());
    ModuleManager::AttachModule(std::make_unique<ChildObjFgModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjRangeBgModule>());
    ModuleManager::AttachModule(std::make_unique<ChildObjRangeFgModule>());
    ModuleManager::AttachModule(std::make_unique<DLLModule>());
    ModuleManager::AttachModule(std::make_unique<DebugModule>());
    ModuleManager::AttachModule(std::make_unique<EventLoopModule>());
    ModuleManager::AttachModule(std::make_unique<G00Module>());
    ModuleManager::AttachModule(std::make_unique<GanBgModule>());
    ModuleManager::AttachModule(std::make_unique<GanFgModule>());
    ModuleManager::AttachModule(std::make_unique<GrpModule>());
    ModuleManager::AttachModule(std::make_unique<JmpModule>());
    ModuleManager::AttachModule(std::make_unique<KoeModule>());
    ModuleManager::AttachModule(std::make_unique<LayeredShakingModule>());
    ModuleManager::AttachModule(std::make_unique<MemModule>());
    ModuleManager::AttachModule(std::make_unique<MovModule>());
    ModuleManager::AttachModule(std::make_unique<MsgModule>());
    ModuleManager::AttachModule(std::make_unique<ObjBgCreationModule>());
    ModuleManager::AttachModule(std::make_unique<ObjBgGettersModule>());
    ModuleManager::AttachModule(std::make_unique<ObjBgManagement>());
    ModuleManager::AttachModule(std::make_unique<ObjBgModule>());
    ModuleManager::AttachModule(std::make_unique<ObjFgCreationModule>());
    ModuleManager::AttachModule(std::make_unique<ObjFgGettersModule>());
    ModuleManager::AttachModule(std::make_unique<ObjFgManagement>());
    ModuleManager::AttachModule(std::make_unique<ObjFgModule>());
    ModuleManager::AttachModule(std::make_unique<ObjManagement>());
    ModuleManager::AttachModule(std::make_unique<ObjRangeBgModule>());
    ModuleManager::AttachModule(std::make_unique<ObjRangeFgModule>());
    ModuleManager::AttachModule(std::make_unique<OsModule>());
    ModuleManager::AttachModule(std::make_unique<PcmModule>());
    ModuleManager::AttachModule(std::make_unique<RefreshModule>());
    ModuleManager::AttachModule(std::make_unique<ScrModule>());
    ModuleManager::AttachModule(std::make_unique<SeModule>());
    ModuleManager::AttachModule(std::make_unique<SelModule>());
    ModuleManager::AttachModule(std::make_unique<ShakingModule>());
    ModuleManager::AttachModule(std::make_unique<StrModule>());
    ModuleManager::AttachModule(std::make_unique<SysModule>());
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}();

}  // namespace
