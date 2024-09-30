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

#include "machine/rlmachine.h"

#include "modules/module_bgm.h"
#include "modules/module_bgr.h"
#include "modules/module_debug.h"
#include "modules/module_dll.h"
#include "modules/module_event_loop.h"
#include "modules/module_g00.h"
#include "modules/module_gan.h"
#include "modules/module_grp.h"
#include "modules/module_jmp.h"
#include "modules/module_koe.h"
#include "modules/module_mem.h"
#include "modules/module_mov.h"
#include "modules/module_msg.h"
#include "modules/module_obj_creation.h"
#include "modules/module_obj_fg_bg.h"
#include "modules/module_obj_getters.h"
#include "modules/module_obj_management.h"
#include "modules/module_os.h"
#include "modules/module_pcm.h"
#include "modules/module_refresh.h"
#include "modules/module_scr.h"
#include "modules/module_se.h"
#include "modules/module_sel.h"
#include "modules/module_shk.h"
#include "modules/module_shl.h"
#include "modules/module_str.h"
#include "modules/module_sys.h"
#include "systems/base/graphics_system.h"
#include "systems/base/system.h"

void AddAllModules(IModuleManager& manager) {
  // Attatch the modules for some commands
  manager.AttachModule(new BgmModule);
  manager.AttachModule(new BgrModule);
  manager.AttachModule(new BraModule);
  manager.AttachModule(new ChildGanBgModule);
  manager.AttachModule(new ChildGanFgModule);
  manager.AttachModule(new ChildObjBgCreationModule);
  manager.AttachModule(new ChildObjBgGettersModule);
  manager.AttachModule(new ChildObjBgManagement);
  manager.AttachModule(new ChildObjBgModule);
  manager.AttachModule(new ChildObjFgCreationModule);
  manager.AttachModule(new ChildObjFgGettersModule);
  manager.AttachModule(new ChildObjFgManagement);
  manager.AttachModule(new ChildObjFgModule);
  manager.AttachModule(new ChildObjRangeBgModule);
  manager.AttachModule(new ChildObjRangeFgModule);
  manager.AttachModule(new DLLModule);
  manager.AttachModule(new DebugModule);
  manager.AttachModule(new EventLoopModule);
  manager.AttachModule(new G00Module);
  manager.AttachModule(new GanBgModule);
  manager.AttachModule(new GanFgModule);
  manager.AttachModule(new GrpModule);
  manager.AttachModule(new JmpModule);
  manager.AttachModule(new KoeModule);
  manager.AttachModule(new LayeredShakingModule);
  manager.AttachModule(new MemModule);
  manager.AttachModule(new MovModule);
  manager.AttachModule(new MsgModule);
  manager.AttachModule(new ObjBgCreationModule);
  manager.AttachModule(new ObjBgGettersModule);
  manager.AttachModule(new ObjBgManagement);
  manager.AttachModule(new ObjBgModule);
  manager.AttachModule(new ObjFgCreationModule);
  manager.AttachModule(new ObjFgGettersModule);
  manager.AttachModule(new ObjFgManagement);
  manager.AttachModule(new ObjFgModule);
  manager.AttachModule(new ObjManagement);
  manager.AttachModule(new ObjRangeBgModule);
  manager.AttachModule(new ObjRangeFgModule);
  manager.AttachModule(new OsModule);
  manager.AttachModule(new PcmModule);
  manager.AttachModule(new RefreshModule);
  manager.AttachModule(new ScrModule);
  manager.AttachModule(new SeModule);
  manager.AttachModule(new SelModule);
  manager.AttachModule(new ShakingModule);
  manager.AttachModule(new StrModule);
  manager.AttachModule(new SysModule);
}
