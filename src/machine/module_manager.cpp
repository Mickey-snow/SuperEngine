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

  const int module_type = mod->module_type();
  const int module_number = mod->module_number();

  const auto hash = std::make_pair(module_type, module_number);
  auto result = modules_.try_emplace(hash, mod);
  if (!result.second) {
    throw std::runtime_error("ModuleManager::AttachModule: Module " +
                             mod->module_name() + " already attached.");
  }

  for (const auto& [op_key, op] : mod->GetStoredOperations()) {
    const int opcode = op_key.first;
    const int overload = op_key.second;
    std::string op_name = op->Name();

    cmd2operation_[std::make_tuple(module_type, module_number, opcode,
                                   overload)] = op;
    name2operation_.emplace(op_name, op);
  }
}

std::shared_ptr<RLModule> ModuleManager::GetModule(int module_type,
                                                   int module_id) const {
  auto it = modules_.find(std::make_pair(module_type, module_id));
  if (it != modules_.cend())
    return it->second;
  return nullptr;
}

std::shared_ptr<RLOperation> ModuleManager::Dispatch(
    const libreallive::CommandElement& cmd) const {
  return GetOperation(cmd.modtype(), cmd.module(), cmd.opcode(),
                      cmd.overload());
}

std::string ModuleManager::GetCommandName(
    const libreallive::CommandElement& f) const {
  auto op = Dispatch(f);
  if (op == nullptr)
    return {};
  return op->Name();
}

std::shared_ptr<RLOperation> ModuleManager::GetOperation(int module_type,
                                                         int module_id,
                                                         int opcode,
                                                         int overload) const {
  auto key = std::make_tuple(module_type, module_id, opcode, overload);
  if (!cmd2operation_.contains(key))
    return nullptr;
  else
    return cmd2operation_.at(key);
}

ModuleManager ModuleManager::CreatePrototype() {
  ModuleManager prototype;

  try {
    prototype.AttachModule(std::make_shared<BgmModule>());
    prototype.AttachModule(std::make_shared<BgrModule>());
    prototype.AttachModule(std::make_shared<BraModule>());
    prototype.AttachModule(std::make_shared<ChildGanBgModule>());
    prototype.AttachModule(std::make_shared<ChildGanFgModule>());
    prototype.AttachModule(std::make_shared<ChildObjBgCreationModule>());
    prototype.AttachModule(std::make_shared<ChildObjBgGettersModule>());
    prototype.AttachModule(std::make_shared<ChildObjBgManagement>());
    prototype.AttachModule(std::make_shared<ChildObjBgModule>());
    prototype.AttachModule(std::make_shared<ChildObjFgCreationModule>());
    prototype.AttachModule(std::make_shared<ChildObjFgGettersModule>());
    prototype.AttachModule(std::make_shared<ChildObjFgManagement>());
    prototype.AttachModule(std::make_shared<ChildObjFgModule>());
    prototype.AttachModule(std::make_shared<ChildObjRangeBgModule>());
    prototype.AttachModule(std::make_shared<ChildObjRangeFgModule>());
    prototype.AttachModule(std::make_shared<DLLModule>());
    prototype.AttachModule(std::make_shared<DebugModule>());
    prototype.AttachModule(std::make_shared<EventLoopModule>());
    prototype.AttachModule(std::make_shared<G00Module>());
    prototype.AttachModule(std::make_shared<GanBgModule>());
    prototype.AttachModule(std::make_shared<GanFgModule>());
    prototype.AttachModule(std::make_shared<GrpModule>());
    prototype.AttachModule(std::make_shared<JmpModule>());
    prototype.AttachModule(std::make_shared<KoeModule>());
    prototype.AttachModule(std::make_shared<LayeredShakingModule>());
    prototype.AttachModule(std::make_shared<MemModule>());
    prototype.AttachModule(std::make_shared<MovModule>());
    prototype.AttachModule(std::make_shared<MsgModule>());
    prototype.AttachModule(std::make_shared<ObjBgCreationModule>());
    prototype.AttachModule(std::make_shared<ObjBgGettersModule>());
    prototype.AttachModule(std::make_shared<ObjBgManagement>());
    prototype.AttachModule(std::make_shared<ObjBgModule>());
    prototype.AttachModule(std::make_shared<ObjFgCreationModule>());
    prototype.AttachModule(std::make_shared<ObjFgGettersModule>());
    prototype.AttachModule(std::make_shared<ObjFgManagement>());
    prototype.AttachModule(std::make_shared<ObjFgModule>());
    prototype.AttachModule(std::make_shared<ObjManagement>());
    prototype.AttachModule(std::make_shared<ObjRangeBgModule>());
    prototype.AttachModule(std::make_shared<ObjRangeFgModule>());
    prototype.AttachModule(std::make_shared<OsModule>());
    prototype.AttachModule(std::make_shared<PcmModule>());
    prototype.AttachModule(std::make_shared<RefreshModule>());
    prototype.AttachModule(std::make_shared<ScrModule>());
    prototype.AttachModule(std::make_shared<SeModule>());
    prototype.AttachModule(std::make_shared<SelModule>());
    prototype.AttachModule(std::make_shared<ShakingModule>());
    prototype.AttachModule(std::make_shared<StrModule>());
    prototype.AttachModule(std::make_shared<SysModule>());
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return prototype;
};
