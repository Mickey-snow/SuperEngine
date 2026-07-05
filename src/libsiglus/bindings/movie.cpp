// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "libsiglus/bindings/registry.hpp"

#include "core/asset_scanner.hpp"
#include "core/avdec/ffmpeg.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "libsiglus/siglus_runtime.hpp"
#include "srbind/module.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/sdl/sdl_surface.hpp"
#include "systems/sound_system.hpp"
#include "systems/system.hpp"
#include "vm/future.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;
namespace fs = std::filesystem;
namespace chr = std::chrono;

namespace {

const std::set<std::string> kMovieExtensions{"wmv", "asf", "avi", "mpg",
                                             "mpeg"};

struct MovieRect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

class SiglusMovie {
 public:
  explicit SiglusMovie(SiglusRuntime& runtime)
      : vm_(runtime.vm.get()),
        system_(runtime.system.get()),
        asset_scanner_(runtime.asset_scanner) {}

  struct MoviePlayer : public CoroutineTask {
    MoviePlayer(sr::VM& vm,
                System* sys,
                fs::path pth,
                MovieRect rect,
                int volume,
                bool wait_key)
        : CoroutineTask(vm, sys->event_ptr().get()),
          path_(std::move(pth)),
          rect_(rect),
          wait_key_(wait_key),
          volume_(volume),
          decoder_(path_),
          clock_(sys->event().GetClock()),
          sound_(sys->sound()),
          graphics_(sys->graphics_ptr()),
          frame_(static_cast<std::size_t>(decoder_.width()) *
                     decoder_.height() * 4,
                 0),
          surface_(graphics_->CreateSurfaceBGRA(decoder_.info().size,
                                                frame_,
                                                false)) {}

    TaskCoroutine Run() override {
      playing_ = true;
      start_ = clock_->GetTime();

      try {
        sound_.PlayMovieAudio(path_, volume_);
      } catch (const std::exception&) {
        sound_.StopMovieAudio();
      }

      TakeGraphicsUpdateResponsibility();

      try {
        while (playing_) {
          const int ms = CurrentTimeMs();
          if (!decoder_.IsPlaying(ms))
            break;

          if (decoder_.DecodeTime(ms, frame_, false))
            surface_->UpdateBGRA(frame_, false);

          graphics_->RenderCustomFrame([this] {
            surface_->RenderToScreen(surface_->GetRect(), DestinationRect(),
                                     255);
          });

          if ((co_await WaitFor(chr::milliseconds(20), wait_key_) ==
               WaitOutcome::InterruptedByInput)) {
            Stop();
            co_return 1;
          }
        }
      } catch (...) {
        Stop();
        throw;
      }

      Stop();
      co_return 0;
    }

    int CurrentTimeMs() const {
      if (sound_.MovieAudioPlaying())
        return std::max(0, sound_.MovieAudioTimeMs());

      const Clock::timepoint_t now = clock_->GetTime();
      const auto duration = chr::duration_cast<chr::milliseconds>(now - start_);
      const auto ms = duration.count();
      return ms > 0 ? static_cast<int>(ms) : 0;
    }

    bool IsPlaying() const {
      return playing_ && decoder_.IsPlaying(CurrentTimeMs());
    }

    void Stop() {
      if (stopped_)
        return;
      stopped_ = true;
      playing_ = false;
      RestoreGraphicsUpdateResponsibility();
      sound_.StopMovieAudio();
    }

    Rect DestinationRect() const {
      const Rect source = surface_->GetRect();
      const int width = rect_.width > 0 ? rect_.width : source.width();
      const int height = rect_.height > 0 ? rect_.height : source.height();
      return Rect::REC(rect_.x, rect_.y, width, height);
    }

    void TakeGraphicsUpdateResponsibility() {
      if (previous_graphics_update_responsibility_)
        return;
      previous_graphics_update_responsibility_ =
          graphics_->is_responsible_for_update();
      graphics_->set_is_responsible_for_update(false);
    }

    void RestoreGraphicsUpdateResponsibility() {
      if (!previous_graphics_update_responsibility_)
        return;
      graphics_->set_is_responsible_for_update(
          *previous_graphics_update_responsibility_);
      previous_graphics_update_responsibility_.reset();
    }

    fs::path path_;
    MovieRect rect_;
    bool wait_key_;
    int volume_ = 255;
    bool playing_ = false;
    bool stopped_ = false;
    mutable FfmpegVideoDecoder decoder_;
    std::shared_ptr<Clock> clock_;
    SoundSystem& sound_;
    std::shared_ptr<GraphicsSystem> graphics_;
    std::vector<char> frame_;
    std::shared_ptr<SDLSurface> surface_;
    Clock::timepoint_t start_;
    std::optional<bool> previous_graphics_update_responsibility_;
  };

  MovieRect DefaultRect() const {
    MovieRect rect;
    if (system_) {
      Size size = system_->graphics().screen_size();
      rect.width = size.width();
      rect.height = size.height();
    }
    return rect;
  }

  void Play(std::string file_name,
            std::optional<MovieRect> maybe_rect,
            bool key_skip = false) {
    MovieRect rect = DefaultRect();
    rect = maybe_rect.value_or(rect);

    Stop();
    if (!vm_ || !system_ || !asset_scanner_)
      return;

    fs::path path = FindMovie(file_name);

    if (system_->ShouldFastForward()) {
      FfmpegVideoDecoder validate(path);
      return;
    }

    std::unique_ptr<MoviePlayer> player = std::make_unique<MoviePlayer>(
        *vm_, system_, std::move(path), rect, volume_, key_skip);
    player_ = player.get();
    pending_ = FutureBackedCoroutineTask(std::move(player));
    pending_->Start();
  }

  bool IsPlaying() const {
    if (!player_)
      return false;
    return player_->IsPlaying();
  }

  sr::Value Wait(sr::VM& vm, bool key_skip) {
    if (!player_ || !pending_)
      return MakeResolvedFuture(*vm.gc_);
    player_->wait_key_ = key_skip;
    sr::Future* fut = pending_->MakeFuture(*vm.gc_);
    return sr::Value(fut);
  }

  void Stop() {
    if (!player_)
      return;
    player_->Stop();
    player_ = nullptr;
    pending_.reset();
  }

  std::filesystem::path FindMovie(const std::string& file_name) const {
    if (!asset_scanner_)
      throw std::runtime_error("mov.play requires an asset scanner");

    auto path = asset_scanner_->FindFile(file_name, kMovieExtensions);
    if (!path) {
      throw std::runtime_error("mov.play could not find " + file_name + ": " +
                               path.error().what());
    }
    return path.value();
  }

  sr::VM* vm_ = nullptr;
  System* system_ = nullptr;
  std::shared_ptr<AssetScanner> asset_scanner_;
  int volume_ = 255;
  MoviePlayer* player_ = nullptr;
  std::optional<FutureBackedCoroutineTask> pending_;
};

}  // namespace

static std::pair<std::string, std::optional<MovieRect>> ParsePlayArgs(
    std::vector<sr::Value> args) {
  CallPacket packet = CallPacket::DecodeFrom(std::move(args));
  if (packet.args.empty())
    throw std::runtime_error("mov.play expects a movie name");

  std::optional<MovieRect> rect;

  std::string file_name = AsString(packet.args[0]);
  if (file_name.empty())
    throw std::runtime_error("mov.play movie name is empty");

  if (packet.args.size() >= 5) {
    rect = {};
    rect->x = RequireInt(packet.args[1], "mov.play x");
    rect->y = RequireInt(packet.args[2], "mov.play y");
    rect->width = RequireInt(packet.args[3], "mov.play width");
    rect->height = RequireInt(packet.args[4], "mov.play height");
  }

  return {std::move(file_name), rect};
}

void BindMovie(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;
  auto movie = std::make_shared<SiglusMovie>(runtime);

  sb::module_ m(vm, "mov");
  m.def(
      "play",
      [movie](std::vector<sr::Value> args) {
        auto [file_name, rect] = ParsePlayArgs(std::move(args));
        movie->Play(std::move(file_name), rect);
      },
      sb::vararg);
  m.def(
      "play_wait",
      [movie](sr::VM& vm, std::vector<sr::Value> args) {
        auto [file_name, rect] = ParsePlayArgs(std::move(args));
        movie->Play(std::move(file_name), rect);
        return movie->Wait(vm, false);
      },
      sb::vararg);
  m.def(
      "play_waitkey",
      [movie](sr::VM& vm, std::vector<sr::Value> args) {
        auto [file_name, rect] = ParsePlayArgs(std::move(args));
        movie->Play(std::move(file_name), rect, true);
        return movie->Wait(vm, true);
      },
      sb::vararg);
  m.def("stop", [movie] { movie->Stop(); });
}

RLVM_REGISTER(SiglusBindingRegistry, "movie", BindMovie)

}  // namespace libsiglus::binding
