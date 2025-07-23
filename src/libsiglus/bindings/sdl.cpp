// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "GL/glew.h"

#include "libsiglus/bindings/sdl.hpp"

#include "vm/vm.hpp"

#include <SDL/SDL.h>

#include <sstream>

namespace libsiglus::binding {
namespace sr = serilang;

void SDL::Bind(sr::VM& vm) {
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  sr::Class* sdl = gc->Allocate<sr::Class>();
  sdl->name = "sdl";
  sdl->methods.try_emplace(
      "__init__",
      gc->Allocate<sr::NativeFunction>(
          "__init__",
          [](sr::Fiber&, std::vector<sr::Value>,
             std::unordered_map<std::string, sr::Value>) -> sr::Value {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
              throw std::runtime_error(
                  std::string("SDL: video initialization failed: ") +
                  SDL_GetError());
            }

            const SDL_VideoInfo* info = SDL_GetVideoInfo();
            if (!info) {
              std::ostringstream ss;
              ss << "Video query failed: " << SDL_GetError();
              throw std::runtime_error(ss.str());
            }

            // the flags to pass to SDL_SetVideoMode
            int video_flags;
            video_flags = SDL_OPENGL;            // Enable OpenGL in SDL
            video_flags |= SDL_GL_DOUBLEBUFFER;  // Enable double buffering
            video_flags |= SDL_SWSURFACE;
            video_flags |= SDL_RESIZABLE;

            // Sets up OpenGL double buffering
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            SDL_Surface* screen = SDL_SetVideoMode(
                1920, 1080, info->vfmt->BitsPerPixel, video_flags);
            if (!screen) {
              std::ostringstream ss;
              ss << "Video mode set failed: " << SDL_GetError();
              throw std::runtime_error(ss.str());
            }

            GLenum err = glewInit();
            if (GLEW_OK != err) {
              std::ostringstream oss;
              oss << "Failed to initialize GLEW: " << glewGetErrorString(err);
              throw std::runtime_error(oss.str());
            }

            return sr::Value();
          }));

  vm.globals_->map["sdl"] = sr::Value(sdl);
}

}  // namespace libsiglus::binding
