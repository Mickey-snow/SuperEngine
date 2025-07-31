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

class SDL_siglus {
  std::shared_ptr<SDLSoundImpl> sound_impl;
  std::shared_ptr<AssetScanner> scanner;

 public:
  SDL_siglus() : sound_impl(nullptr), scanner(nullptr) {}

  void init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      throw std::runtime_error(
          std::string("SDL: video initialization failed: ") + SDL_GetError());
    }

    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    if (!info) {
      std::ostringstream ss;
      ss << "Video query failed: " << SDL_GetError();
      throw std::runtime_error(ss.str());
    }

    // the flags to pass to SDL_SetVideoMode
    const int video_flags = SDL_OPENGL | SDL_RESIZABLE;

    // Sets up OpenGL double buffering
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Surface* screen =
        SDL_SetVideoMode(1920, 1080, info->vfmt->BitsPerPixel, video_flags);
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

    sound_impl = std::make_shared<SDLSoundImpl>();
    sound_impl->InitSystem();
    sound_impl->OpenAudio(AVSpec{.sample_rate = 48000,
                                 .sample_format = AV_SAMPLE_FMT::S16,
                                 .channel_count = 2},
                          4096);
    sound_impl->AllocateChannels(32);

    scanner = std::make_shared<AssetScanner>();
    scanner->IndexDirectory(fs::path("/") / "tmp" / "audio");
  }

  void play(std::string name) {
    fs::path path = scanner->FindFile(name);
    player_t player = CreateAudioPlayer(path);

    sound_impl->SetVolume(2, 127);
    sound_impl->PlayChannel(2, player);
  }

  void bgm(std::string name) {
    fs::path path = scanner->FindFile(name);
    player_t player = CreateAudioPlayer(path);

    sound_impl->EnableBgm();
    sound_impl->PlayBgm(player);
  }
};

class Combined : public sr::Class, public SDL_siglus {
 public:
  template <typename... Ts>
  Combined(Ts&&... params)
      : sr::Class(), SDL_siglus(std::forward<Ts>(params)...) {}
};

void SDL::Bind(sr::VM& vm) {
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  auto* bind = gc->Allocate<Combined>();
  vm.globals_->map["sdl"] = sr::Value(bind);
  bind->name = "sdl";
  bind->methods.try_emplace(
      "__init__",
      gc->Allocate<sr::NativeFunction>(
          "__init__",
          [bind](sr::Fiber&, std::vector<sr::Value>,
                 std::unordered_map<std::string, sr::Value>) -> sr::Value {
            bind->init();
            return sr::nil;
          }));
  bind->methods.try_emplace(
      "play",
      gc->Allocate<sr::NativeFunction>(
          "play",
          [bind](sr::Fiber&, std::vector<sr::Value> args,
                 std::unordered_map<std::string, sr::Value>) -> sr::Value {
            std::string* name;
            if (args.size() < 1 ||
                ((name = args.front().Get_if<std::string>()) == nullptr))
              throw std::runtime_error(
                  "Play: first argument 'file_name' must be string");

            bind->play(*name);
            return sr::Value(true);
          }));
  bind->methods.try_emplace(
      "bgm",
      gc->Allocate<sr::NativeFunction>(
          "bgm",
          [bind](sr::Fiber&, std::vector<sr::Value> args,
                 std::unordered_map<std::string, sr::Value>) -> sr::Value {
            std::string* name;
            if (args.size() < 1 ||
                ((name = args.front().Get_if<std::string>()) == nullptr))
              throw std::runtime_error(
                  "Bgm: first argument 'file_name' must be string");

            bind->bgm(*name);
            return sr::Value(true);
          }));
}

}  // namespace libsiglus::binding
