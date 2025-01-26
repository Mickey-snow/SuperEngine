// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "systems/sdl/sdl_system.hpp"

#include <SDL/SDL.h>

#include <sstream>

#include "core/gameexe.hpp"
#include "core/rlevent_listener.hpp"
#include "libreallive/alldefs.hpp"
#include "machine/rlmachine.hpp"
#include "object/objdrawer.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/platform.hpp"
#include "systems/sdl/sdl_event_system.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_sound_system.hpp"
#include "systems/sdl/sdl_text_system.hpp"
#include "systems/sdl/sound_implementor.hpp"

// -----------------------------------------------------------------------

SDLSystem::SDLSystem(Gameexe& gameexe) : System(), gameexe_(gameexe) {
  // First, initialize SDL's video subsystem.
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::ostringstream ss;
    ss << "Video initialization failed: " << SDL_GetError();
    throw libreallive::Error(ss.str());
  }

  // Initialize the various subsystems
  graphics_system_ = std::make_shared<SDLGraphicsSystem>(*this, gameexe);
  event_system_ = std::make_shared<SDLEventSystem>();
  text_system_ = std::make_shared<SDLTextSystem>(*this, gameexe);

  // The implementor for sound system
  auto sound_impl = std::make_unique<SDLSoundImpl>();
  // Currently only sound system is refactored to use the bridge pattern, other
  // subsystem's implementation are bound permanently with their abstraction,
  // meaning all classes in systems/sdl needed to be reimplemented for a new
  // system or different implementation.
  sound_system_ =
      std::make_shared<SDLSoundSystem>(*this, std::move(sound_impl));

  event_system_->AddListener(graphics_system_);
  event_system_->AddListener(text_system_);

  event_system_->AddListener(19, rlevent_handler_);
}

// -----------------------------------------------------------------------

SDLSystem::~SDLSystem() {
  // Some combinations of SDL and FT on the Mac require us to destroy the
  // Platform first. This will crash on Tiger if this isn't here, but it won't
  // crash under Linux...
  platform_.reset();

  // Force the deletion of the various systems before we shut down
  // SDL.
  sound_system_.reset();
  graphics_system_.reset();
  event_system_.reset();
  text_system_.reset();

  SDL_Quit();
}

// -----------------------------------------------------------------------

void SDLSystem::Run(RLMachine& machine) {
  // Give the event handler a chance to run.
  event_system_->ExecuteEventSystem(machine);
  text_system_->ExecuteTextSystem();
  sound_system_->ExecuteSoundSystem();
  graphics_system_->ExecuteGraphicsSystem(machine);

  if (platform())
    platform()->Run(machine);
}

// -----------------------------------------------------------------------

GraphicsSystem& SDLSystem::graphics() { return *graphics_system_; }

// -----------------------------------------------------------------------

EventSystem& SDLSystem::event() { return *event_system_; }

// -----------------------------------------------------------------------

Gameexe& SDLSystem::gameexe() { return gameexe_; }

// -----------------------------------------------------------------------

SDLTextSystem& SDLSystem::text() { return *text_system_; }

// -----------------------------------------------------------------------

SoundSystem& SDLSystem::sound() { return *sound_system_; }

SDLGraphicsSystem* getSDLGraphics(System& system) {
  return static_cast<SDLGraphicsSystem*>(&system.graphics());
}
