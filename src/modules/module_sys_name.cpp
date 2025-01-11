// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2008 Elliot Glaysher
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

#include "modules/module_sys_name.hpp"

#include <string>

#include "machine/rlmachine.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/reference_types.hpp"
#include "memory/memory.hpp"

// -----------------------------------------------------------------------

namespace {

struct GetName : public RLOpcode<IntConstant_T, StrReference_T> {
  void operator()(RLMachine& machine,
                  int index,
                  StringReferenceIterator strIt) {
    *strIt = machine.GetMemory().Read(StrBank::global_name, index);
  }
};

struct SetName : public RLOpcode<IntConstant_T, StrConstant_T> {
  void operator()(RLMachine& machine, int index, string name) {
    machine.GetMemory().Write(StrBank::global_name, index, name);
  }
};

struct GetLocalName : public RLOpcode<IntConstant_T, StrReference_T> {
  void operator()(RLMachine& machine,
                  int index,
                  StringReferenceIterator strIt) {
    *strIt = machine.GetMemory().Read(StrBank::local_name, index);
  }
};

struct SetLocalName : public RLOpcode<IntConstant_T, StrConstant_T> {
  void operator()(RLMachine& machine, int index, string name) {
    machine.GetMemory().Write(StrBank::local_name, index, name);
  }
};

}  // namespace

// -----------------------------------------------------------------------

void AddSysNameOpcodes(RLModule& m) {
  m.AddOpcode(1300, 0, "GetName", new GetName);
  m.AddOpcode(1301, 0, "SetName", new SetName);
  m.AddOpcode(1310, 0, "GetLocalName", new GetLocalName);
  m.AddOpcode(1311, 0, "SetLocalName", new SetLocalName);
}
