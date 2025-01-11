// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include "modules/module_sys_syscom.hpp"

#include "machine/general_operations.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/reference_types.hpp"
#include "machine/rloperation/rlop_store.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/system.hpp"

// -----------------------------------------------------------------------

namespace {

struct ContextMenu : public RLOpcode<> {
  void operator()(RLMachine& machine) {
    // Based off of ALMA, it appears that we also are responsible for flushing
    // clicks.
    machine.GetSystem().event().FlushMouseClicks();

    machine.GetSystem().ShowSyscomMenu(machine);
  }
};

struct SyscomEnabled : public RLStoreOpcode<IntConstant_T> {
  int operator()(RLMachine& machine, int num) {
    return machine.GetSystem().IsSyscomEnabled(num);
  }
};

}  // namespace

// -----------------------------------------------------------------------

void AddSysSyscomOpcodes(RLModule& m) {
  m.AddOpcode(1210, 0, "ContextMenu", new ContextMenu);

  m.AddOpcode(1211, 0, "EnableSyscom",
              CallFunction(&System::EnableSyscomEntry));
  m.AddOpcode(1211, 1, "EnableSyscom", CallFunction(&System::EnableSyscom));

  m.AddOpcode(1212, 0, "HideSyscom", CallFunction(&System::HideSyscomEntry));
  m.AddOpcode(1212, 1, "HideSyscom", CallFunction(&System::HideSyscom));

  m.AddOpcode(1213, 0, "DisableSyscom",
              CallFunction(&System::DisableSyscomEntry));

  m.AddOpcode(1214, 0, "SyscomEnabled", new SyscomEnabled);

  m.AddOpcode(1215, 0, "InvokeSyscom", CallFunction(&System::InvokeSyscom));
  m.AddUnsupportedOpcode(1215, 1, "InvokeSyscom");
  m.AddUnsupportedOpcode(1216, 0, "ReadSyscom");
}
