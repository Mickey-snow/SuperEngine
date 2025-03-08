// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
// Copyright (C) 2024 Serina Sakurai
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

#include "modules/jump.hpp"

#include "machine/call_stack.hpp"
#include "machine/iscriptor.hpp"
#include "machine/rlmachine.hpp"
#include "machine/stack_frame.hpp"

void Jump(RLMachine& machine, int scenario, int entrypoint) {
  auto it = machine.GetCallStack().FindTopRealFrame();
  if (it != nullptr)
    it->pos = machine.GetScriptor()->LoadEntry(scenario, entrypoint);
}

void Farcall(RLMachine& machine, int scenario, int entrypoint) {
  if (entrypoint == 0 && machine.GetScenarioConfig().enable_seentop_savepoint)
    machine.MarkSavepoint();

  auto ip = machine.GetScriptor()->LoadEntry(scenario, entrypoint);
  machine.GetCallStack().Push(
      StackFrame(std::move(ip), StackFrame::TYPE_FARCALL));
}

void Goto(RLMachine& machine, unsigned long loc) {
  auto top_frame = machine.GetCallStack().Top();
  top_frame->pos = machine.GetScriptor()->Load(machine.SceneNumber(), loc);
}

void Gosub(RLMachine& machine, unsigned long loc) {
  auto ip = machine.GetScriptor()->Load(machine.SceneNumber(), loc);
  machine.GetCallStack().Push(
      StackFrame(std::move(ip), StackFrame::TYPE_GOSUB));
}

void Return(RLMachine& machine) { machine.GetCallStack().Pop(); }

void ClearLongOperationsOffBackOfStack(RLMachine& machine) {
  auto& stack = machine.GetCallStack();
  while (stack.Size() && stack.Top() != stack.FindTopRealFrame())
    stack.Pop();
}
