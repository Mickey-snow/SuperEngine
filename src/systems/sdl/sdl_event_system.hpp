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

#pragma once

#include "core/rect.hpp"
#include "systems/base/event_system.hpp"

#include <SDL/SDL_events.h>

class SDLSystem;

class SDLEventSystem : public EventSystem {
 public:
  SDLEventSystem();

  virtual void ExecuteEventSystem(RLMachine& machine) override;

 private:
  // RealLive event system commands
  void HandleKeyDown(RLMachine& machine, SDL_Event& event);
  void HandleKeyUp(RLMachine& machine, SDL_Event& event);
  void HandleMouseMotion(RLMachine& machine, SDL_Event& event);
  void HandleMouseButtonEvent(RLMachine& machine, SDL_Event& event);
  void HandleActiveEvent(RLMachine& machine, SDL_Event& event);
};
