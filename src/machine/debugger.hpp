// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include "core/event_listener.hpp"
#include "machine/instruction.hpp"

#include "m6/script_engine.hpp"

class RLMachine;

class Debugger : public EventListener {
 public:
  Debugger(std::shared_ptr<RLMachine> machine);

  void Execute();

  // Overridden from EventListener
  void OnEvent(std::shared_ptr<Event> event) override;

 private:
  std::shared_ptr<RLMachine> machine_;

  m6::ScriptEngine engine_;

  bool should_break_ = false;
};
