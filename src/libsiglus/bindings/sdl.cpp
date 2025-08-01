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

#include "core/asset_scanner.hpp"
#include "systems/sdl/sound_implementor.hpp"
#include "vm/vm.hpp"

#include <SDL/SDL.h>

#include <filesystem>
#include <sstream>

namespace libsiglus::binding {
namespace sr = serilang;
namespace fs = std::filesystem;

void SDL::Bind(sr::VM& vm) {
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  // TODO: this needs to be added as a field, but the current object system
  // cannot hold native instances yet.
  static SDLSoundImpl* sound_impl = nullptr;
  static AssetScanner* scanner = nullptr;

  sr::Class* sdl = gc->Allocate<sr::Class>();
  sdl->name = "sdl";
  sdl->memfns.try_emplace(
      "__init__",
      gc->Allocate<sr::NativeFunction>(
          "__init__",
          [](sr::VM& vm, sr::Fiber&, std::vector<sr::Value>,
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

            sound_impl = new SDLSoundImpl();
            sound_impl->InitSystem();
            sound_impl->OpenAudio(AVSpec{.sample_rate = 48000,
                                         .sample_format = AV_SAMPLE_FMT::S16,
                                         .channel_count = 2},
                                  4096);
            sound_impl->AllocateChannels(32);

            scanner = new AssetScanner();
            scanner->IndexDirectory(fs::path("/") / "tmp" / "audio");

            return sr::Value(true);
          }));
  sdl->memfns.try_emplace(
      "play",
      gc->Allocate<sr::NativeFunction>(
          "play",
          [](sr::VM& vm, sr::Fiber&, std::vector<sr::Value> args,
             std::unordered_map<std::string, sr::Value>) -> sr::Value {
            std::string* name;
            if (args.size() < 1 ||
                ((name = args.front().Get_if<std::string>()) == nullptr))
              throw std::runtime_error(
                  "Play: first argument 'file_name' must be string");

            fs::path path = scanner->FindFile(*name);
            player_t player = CreateAudioPlayer(path);

            sound_impl->SetVolume(2, 127);
            sound_impl->PlayChannel(2, player);

            return sr::Value(true);
          }));
  sdl->memfns.try_emplace(
      "bgm", gc->Allocate<sr::NativeFunction>(
                 "bgm",
                 [](sr::VM& vm, sr::Fiber&, std::vector<sr::Value> args,
                    std::unordered_map<std::string, sr::Value>) -> sr::Value {
                   std::string* name;
                   if (args.size() < 1 ||
                       ((name = args.front().Get_if<std::string>()) == nullptr))
                     throw std::runtime_error(
                         "Bgm: first argument 'file_name' must be string");

                   fs::path path = scanner->FindFile(*name);
                   player_t player = CreateAudioPlayer(path);

                   sound_impl->EnableBgm();
                   sound_impl->PlayBgm(player);

                   return sr::Value(true);
                 }));

  vm.globals_->map["sdl"] = sr::Value(sdl);
}

}  // namespace libsiglus::binding
