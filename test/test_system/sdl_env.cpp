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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "test_system/sdl_env.hpp"

#include <GL/glew.h>
#include <SDL/SDL.h>

#include <stdexcept>

sdlEnv::sdlEnv() {
  std::string error;
  if (SDL_SetVideoMode(128, 128, 32, SDL_OPENGL) == NULL) {
    error += "Failed to setup sdl video: ";
    error += SDL_GetError();
    SDL_Quit();
  }

  if (!error.empty())
    throw std::runtime_error(error);

  auto glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    error += "GLEW Initialization failed: ";
    error += reinterpret_cast<const char*>(glewGetErrorString(glew_status));
    SDL_Quit();
  }

  if (!error.empty())
    throw std::runtime_error(error);
}

sdlEnv::~sdlEnv() { SDL_Quit(); }

std::shared_ptr<sdlEnv> SetupSDL() {
  static std::weak_ptr<sdlEnv> cached;
  std::shared_ptr<sdlEnv> env = cached.lock();
  if (env)
    return env;

  cached = env = std::make_shared<sdlEnv>();
  return env;
}
