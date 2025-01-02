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

void ModuleManager::AttachModule(std::shared_ptr<RLModule> mod) {
  if (!mod)
    return;

  auto result = Get().AttachModule(mod);
  if (!result) {
    throw std::runtime_error(
        "ModuleManager::AttachModule: Unable to attach module " +
        mod->module_name());
  }
}

void ModuleManager::DetachAll() { Get() = Ctx(); }

IModuleManager& ModuleManager::GetInstance() {
  class ModuleManager_impl : public IModuleManager {
   public:
    ModuleManager_impl(Ctx& ctx_in) : ctx(ctx_in) {}

    std::shared_ptr<RLModule> GetModule(int module_type,
                                        int module_id) const override {
      return ctx.GetModule(module_type, module_id);
    }

    std::shared_ptr<RLOperation> Dispatch(
        const libreallive::CommandElement& cmd) const override {
      auto mod = GetModule(cmd.modtype(), cmd.module());
      if (mod == nullptr)
        return nullptr;

      return mod->Dispatch(cmd);
    }

    std::string GetCommandName(
        const libreallive::CommandElement& f) const override {
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
// ModuleManager::Ctx
// -----------------------------------------------------------------------

ModuleManager::Ctx& ModuleManager::Get() {
  static Ctx ctx_;
  return ctx_;
}

std::shared_ptr<RLModule> ModuleManager::Ctx::GetModule(int module_type,
                                                        int module_id) const {
  auto it = modules_.find(std::make_pair(module_type, module_id));
  if (it != modules_.cend())
    return it->second;
  return nullptr;
}

bool ModuleManager::Ctx::AttachModule(std::shared_ptr<RLModule> mod) {
  const auto hash = std::make_pair(mod->module_type(), mod->module_number());
  auto result = Get().modules_.try_emplace(hash, mod);
  return result.second;
}

// -----------------------------------------------------------------------
// Add all modules by default

namespace {
[[maybe_unused]] bool __attach_all_modules_result = []() {
  try {
    ModuleManager::AttachModule(std::make_shared<BgmModule>());
    ModuleManager::AttachModule(std::make_shared<BgrModule>());
    ModuleManager::AttachModule(std::make_shared<BraModule>());
    ModuleManager::AttachModule(std::make_shared<ChildGanBgModule>());
    ModuleManager::AttachModule(std::make_shared<ChildGanFgModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjBgCreationModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjBgGettersModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjBgManagement>());
    ModuleManager::AttachModule(std::make_shared<ChildObjBgModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjFgCreationModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjFgGettersModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjFgManagement>());
    ModuleManager::AttachModule(std::make_shared<ChildObjFgModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjRangeBgModule>());
    ModuleManager::AttachModule(std::make_shared<ChildObjRangeFgModule>());
    ModuleManager::AttachModule(std::make_shared<DLLModule>());
    ModuleManager::AttachModule(std::make_shared<DebugModule>());
    ModuleManager::AttachModule(std::make_shared<EventLoopModule>());
    ModuleManager::AttachModule(std::make_shared<G00Module>());
    ModuleManager::AttachModule(std::make_shared<GanBgModule>());
    ModuleManager::AttachModule(std::make_shared<GanFgModule>());
    ModuleManager::AttachModule(std::make_shared<GrpModule>());
    ModuleManager::AttachModule(std::make_shared<JmpModule>());
    ModuleManager::AttachModule(std::make_shared<KoeModule>());
    ModuleManager::AttachModule(std::make_shared<LayeredShakingModule>());
    ModuleManager::AttachModule(std::make_shared<MemModule>());
    ModuleManager::AttachModule(std::make_shared<MovModule>());
    ModuleManager::AttachModule(std::make_shared<MsgModule>());
    ModuleManager::AttachModule(std::make_shared<ObjBgCreationModule>());
    ModuleManager::AttachModule(std::make_shared<ObjBgGettersModule>());
    ModuleManager::AttachModule(std::make_shared<ObjBgManagement>());
    ModuleManager::AttachModule(std::make_shared<ObjBgModule>());
    ModuleManager::AttachModule(std::make_shared<ObjFgCreationModule>());
    ModuleManager::AttachModule(std::make_shared<ObjFgGettersModule>());
    ModuleManager::AttachModule(std::make_shared<ObjFgManagement>());
    ModuleManager::AttachModule(std::make_shared<ObjFgModule>());
    ModuleManager::AttachModule(std::make_shared<ObjManagement>());
    ModuleManager::AttachModule(std::make_shared<ObjRangeBgModule>());
    ModuleManager::AttachModule(std::make_shared<ObjRangeFgModule>());
    ModuleManager::AttachModule(std::make_shared<OsModule>());
    ModuleManager::AttachModule(std::make_shared<PcmModule>());
    ModuleManager::AttachModule(std::make_shared<RefreshModule>());
    ModuleManager::AttachModule(std::make_shared<ScrModule>());
    ModuleManager::AttachModule(std::make_shared<SeModule>());
    ModuleManager::AttachModule(std::make_shared<SelModule>());
    ModuleManager::AttachModule(std::make_shared<ShakingModule>());
    ModuleManager::AttachModule(std::make_shared<StrModule>());
    ModuleManager::AttachModule(std::make_shared<SysModule>());
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}();

}  // namespace
