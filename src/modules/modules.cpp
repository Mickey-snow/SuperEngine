// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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

#include "machine/rlmachine.hpp"

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
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"

void AddAllModules(IModuleManager& manager) {
  // Attatch the modules for some commands
  manager.AttachModule(std::make_unique<BgmModule>());
  manager.AttachModule(std::make_unique<BgrModule>());
  manager.AttachModule(std::make_unique<BraModule>());
  manager.AttachModule(std::make_unique<ChildGanBgModule>());
  manager.AttachModule(std::make_unique<ChildGanFgModule>());
  manager.AttachModule(std::make_unique<ChildObjBgCreationModule>());
  manager.AttachModule(std::make_unique<ChildObjBgGettersModule>());
  manager.AttachModule(std::make_unique<ChildObjBgManagement>());
  manager.AttachModule(std::make_unique<ChildObjBgModule>());
  manager.AttachModule(std::make_unique<ChildObjFgCreationModule>());
  manager.AttachModule(std::make_unique<ChildObjFgGettersModule>());
  manager.AttachModule(std::make_unique<ChildObjFgManagement>());
  manager.AttachModule(std::make_unique<ChildObjFgModule>());
  manager.AttachModule(std::make_unique<ChildObjRangeBgModule>());
  manager.AttachModule(std::make_unique<ChildObjRangeFgModule>());
  manager.AttachModule(std::make_unique<DLLModule>());
  manager.AttachModule(std::make_unique<DebugModule>());
  manager.AttachModule(std::make_unique<EventLoopModule>());
  manager.AttachModule(std::make_unique<G00Module>());
  manager.AttachModule(std::make_unique<GanBgModule>());
  manager.AttachModule(std::make_unique<GanFgModule>());
  manager.AttachModule(std::make_unique<GrpModule>());
  manager.AttachModule(std::make_unique<JmpModule>());
  manager.AttachModule(std::make_unique<KoeModule>());
  manager.AttachModule(std::make_unique<LayeredShakingModule>());
  manager.AttachModule(std::make_unique<MemModule>());
  manager.AttachModule(std::make_unique<MovModule>());
  manager.AttachModule(std::make_unique<MsgModule>());
  manager.AttachModule(std::make_unique<ObjBgCreationModule>());
  manager.AttachModule(std::make_unique<ObjBgGettersModule>());
  manager.AttachModule(std::make_unique<ObjBgManagement>());
  manager.AttachModule(std::make_unique<ObjBgModule>());
  manager.AttachModule(std::make_unique<ObjFgCreationModule>());
  manager.AttachModule(std::make_unique<ObjFgGettersModule>());
  manager.AttachModule(std::make_unique<ObjFgManagement>());
  manager.AttachModule(std::make_unique<ObjFgModule>());
  manager.AttachModule(std::make_unique<ObjManagement>());
  manager.AttachModule(std::make_unique<ObjRangeBgModule>());
  manager.AttachModule(std::make_unique<ObjRangeFgModule>());
  manager.AttachModule(std::make_unique<OsModule>());
  manager.AttachModule(std::make_unique<PcmModule>());
  manager.AttachModule(std::make_unique<RefreshModule>());
  manager.AttachModule(std::make_unique<ScrModule>());
  manager.AttachModule(std::make_unique<SeModule>());
  manager.AttachModule(std::make_unique<SelModule>());
  manager.AttachModule(std::make_unique<ShakingModule>());
  manager.AttachModule(std::make_unique<StrModule>());
  manager.AttachModule(std::make_unique<SysModule>());
}
