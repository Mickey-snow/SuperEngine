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

#include "systems/graphics_system.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>

#include "core/asset_scanner.hpp"
#include "core/cgm_table.hpp"
#include "core/gameexe.hpp"
#include "core/memory.hpp"
#include "core/object.hpp"
#include "core/object_internal/drawer/anm.hpp"
#include "core/object_internal/drawer/file.hpp"
#include "core/object_internal/objdrawer.hpp"
#include "core/object_internal/object_mutator.hpp"
#include "core/rlevent_listener.hpp"
#include "core/stage.hpp"
#include "libreallive/expression.hpp"
#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "machine/stack_frame.hpp"
#include "modules/module_grp.hpp"
#include "systems/event_system.hpp"
#include "systems/hik_renderer.hpp"
#include "systems/hik_script.hpp"
#include "systems/igraphics_backend.hpp"
#include "systems/mouse_cursor.hpp"
#include "systems/object_settings.hpp"
#include "systems/renderable.hpp"
#include "systems/sdl/sdl_surface.hpp"
#include "systems/system.hpp"
#include "systems/system_error.hpp"
#include "systems/text_system.hpp"
#include "utilities/graphics.hpp"
#include "utilities/lazy_array.hpp"
#include "utilities/string_utilities.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <deque>
#include <format>
#include <set>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// GraphicsSystem::GraphicsObjectSettings
// -----------------------------------------------------------------------
// Impl object
struct GraphicsSystem::GraphicsObjectSettings {
  // Number of graphical objects in a layer.
  int objects_in_a_layer;

  // Each is a valid index into data, associating each object slot with an
  // ObjectSettings instance.
  std::vector<unsigned char> position;

  std::vector<ObjectSettings> data;

  explicit GraphicsObjectSettings(Gameexe& gameexe);

  const ObjectSettings& GetObjectSettingsFor(int obj_num);
};

// -----------------------------------------------------------------------
static std::optional<std::pair<int, int>> parse_range(std::string_view sv) {
  // Accepts "N" or "A:B" (whitespace allowed). Returns nullopt if invalid.
  sv = trim_sv(sv);
  if (auto pos = sv.find(':'); pos == std::string_view::npos) {
    int v;
    if (!parse_int(sv, v))
      return std::nullopt;
    return std::make_pair(v, v);
  } else {
    int a, b;
    if (!parse_int(sv.substr(0, pos), a))
      return std::nullopt;
    if (!parse_int(sv.substr(pos + 1), b))
      return std::nullopt;
    return std::make_pair(a, b);
  }
}
GraphicsSystem::GraphicsObjectSettings::GraphicsObjectSettings(
    Gameexe& gameexe) {
  static constexpr std::vector<int> empty_ivec;

  objects_in_a_layer = gameexe("OBJECT_MAX").Int().value_or(256);

  // First we populate everything with the special value
  position = std::vector<unsigned char>(objects_in_a_layer, 0);

  data.emplace_back(gameexe("OBJECT.999").IntVec().value_or(empty_ivec));

  // Read the #OBJECT.xxx entries from the Gameexe
  for (auto it : gameexe.Filter("OBJECT.")) {
    std::string_view key = it.key();
    size_t dot_pos = key.find('.');
    if (dot_pos == std::string_view::npos || dot_pos + 1 >= key.size())
      continue;

    std::string_view s = key.substr(dot_pos + 1);
    int lo, hi;
    if (auto maybe = parse_range(s); maybe && maybe->first <= maybe->second)
      std::tie(lo, hi) = *maybe;
    else
      continue;

    for (int obj_num : std::views::iota(lo, hi + 1)) {
      if (obj_num != 999 && obj_num < objects_in_a_layer) {
        position[obj_num] = data.size();
        data.emplace_back(it.IntVec().value_or(empty_ivec));
      }
    }
  }
}

// -----------------------------------------------------------------------

const ObjectSettings&
GraphicsSystem::GraphicsObjectSettings::GetObjectSettingsFor(int obj_num) {
  return data.at(position[obj_num]);
}

// -----------------------------------------------------------------------
// GraphicsSystemGlobals
// -----------------------------------------------------------------------
GraphicsSystemGlobals::GraphicsSystemGlobals()
    : show_object_1(false),
      show_object_2(false),
      show_weather(false),
      skip_animations(0),
      screen_mode(1),
      cg_table(),
      tone_curves() {}

GraphicsSystemGlobals::GraphicsSystemGlobals(Gameexe& gameexe)
    : show_object_1(gameexe("INIT_OBJECT1_ONOFF_MOD").Int().value_or(0) ? 0
                                                                        : 1),
      show_object_2(gameexe("INIT_OBJECT2_ONOFF_MOD").Int().value_or(0) ? 0
                                                                        : 1),
      show_weather(gameexe("INIT_WEATHER_ONOFF_MOD").Int().value_or(0) ? 0 : 1),
      skip_animations(0),
      screen_mode(1),
      cg_table(CreateCGMTable(gameexe)),
      tone_curves(CreateToneCurve(gameexe)) {}

// -----------------------------------------------------------------------
// GraphicsSystem
// -----------------------------------------------------------------------
GraphicsSystem::GraphicsSystem(System& system,
                               Gameexe& gameexe,
                               std::shared_ptr<IGraphicsBackend> backend)
    : screen_update_mode_(SCREENUPDATEMODE_AUTOMATIC),
      background_type_(BACKGROUND_DC0),
      screen_needs_refresh_(false),
      is_responsible_for_update_(true),
      display_subtitle_(gameexe("SUBTITLE").Int().value_or(0)),
      interface_hidden_(false),
      globals_(gameexe),
      time_at_last_queue_change_(0),
      graphics_object_settings_(
          std::make_unique<GraphicsObjectSettings>(gameexe)),
      use_custom_mouse_cursor_(gameexe("MOUSE_CURSOR").Exists()),
      show_cursor_from_bytecode_(true),
      cursor_(gameexe("MOUSE_CURSOR").Int().value_or(0)),
      system_(system),
      impl_(backend),
      asset_scanner_(system.GetAssetScanner()),
      preloaded_hik_scripts_(32),
      preloaded_g00_(256),
      image_cache_(10) {
  Size screen_size = GetScreenSize(gameexe);
  bool is_fullscreen = screen_mode() == 0;
  impl_->InitSystem(screen_size, is_fullscreen);
  SetScreenSize(screen_size);
  display_size_ = screen_size;

  window_title_update_interval_ = std::chrono::milliseconds(60);
  last_window_title_update_ =
      window_title_clock_.GetTime() - window_title_update_interval_;

  int name_enc = gameexe("NAME_ENC").Int().value_or(0);
  caption_title_utf8_ = cp932toUTF8(gameexe("CAPTION").ToStr(), name_enc);
  subtitle_.clear();
  subtitle_utf8_.clear();
  current_window_title_.clear();

  std::string initial_title = ComposeWindowTitle();
  impl_->SetWindowTitle(initial_title);
  current_window_title_ = std::move(initial_title);
  impl_->ShowSystemCursor(!ShouldUseCustomCursor());

  haikei_ = impl_->CreateSurface(screen_size);
  for (int i = 0; i < 16; ++i)
    display_contexts_[i] = impl_->CreateSurface(screen_size);
}

GraphicsSystem::~GraphicsSystem() = default;

// -----------------------------------------------------------------------

void GraphicsSystem::BindStage(Stage* stage) { stage_ = stage; }

// -----------------------------------------------------------------------

Stage& GraphicsSystem::BoundStage() {
  if (!stage_)
    throw std::runtime_error("GraphicsSystem requires a bound stage");
  return *stage_;
}

Stage& GraphicsSystem::BoundStage() const {
  if (!stage_)
    throw std::runtime_error("GraphicsSystem requires a bound stage");
  return *stage_;
}

// -----------------------------------------------------------------------

void GraphicsSystem::ForceRefresh() {
  screen_needs_refresh_ = true;

  if (screen_update_mode_ == SCREENUPDATEMODE_MANUAL) {
    // Note: SDLEventSystem can also set_force_wait(), in the case of automatic
    // mode.
    system().set_force_wait(true);
  }
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetScreenUpdateMode(DCScreenUpdateMode u) {
  screen_update_mode_ = u;
}

// -----------------------------------------------------------------------

void GraphicsSystem::QueueShakeSpec(int spec) {
  Gameexe& gameexe = system().gameexe();

  if (gameexe("SHAKE", spec).Exists()) {
    std::vector<int> spec_vector = gameexe("SHAKE", spec).ToIntVec();

    int x, y, time;
    std::vector<int>::const_iterator it = spec_vector.begin();
    while (it != spec_vector.end()) {
      x = *it++;
      if (it != spec_vector.end()) {
        y = *it++;
        if (it != spec_vector.end()) {
          time = *it++;
          screen_shake_queue_.push(std::make_pair(Point(x, y), time));
        }
      }
    }

    ForceRefresh();
    time_at_last_queue_change_ = system().event().GetTicks();
  }
}

// -----------------------------------------------------------------------

Point GraphicsSystem::GetScreenOrigin() const {
  if (screen_shake_queue_.empty()) {
    return Point(0, 0);
  } else {
    return screen_shake_queue_.front().first;
  }
}

// -----------------------------------------------------------------------

bool GraphicsSystem::IsShaking() const { return !screen_shake_queue_.empty(); }

// -----------------------------------------------------------------------

int GraphicsSystem::CurrentShakingFrameTime() const {
  if (screen_shake_queue_.empty()) {
    return 10;
  } else {
    return screen_shake_queue_.front().second;
  }
}

// -----------------------------------------------------------------------

int GraphicsSystem::ShouldUseCustomCursor() {
  return use_custom_mouse_cursor_ &&
         system().gameexe()("MOUSE_CURSOR", cursor_, "NAME")
                 .Str()
                 .value_or("") != "";
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetCursor(int cursor) {
  cursor_ = cursor;
  mouse_cursor_.reset();
  if (impl_)
    impl_->ShowSystemCursor(!ShouldUseCustomCursor());
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetHikRenderer(HIKRenderer* renderer) {
  hik_renderer_.reset(renderer);
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetWindowSubtitle(const std::string& cp932str,
                                       int text_encoding) {
  subtitle_ = cp932str;
  SetWindowSubtitle(cp932toUTF8(cp932str, text_encoding));
}

void GraphicsSystem::SetWindowSubtitle(std::string utf8str) {
  subtitle_utf8_ = std::move(utf8str);
  // Force update on next tick to avoid stalling title updates after load.
  last_window_title_update_ =
      window_title_clock_.GetTime() - window_title_update_interval_;
  UpdateWindowTitle();
}

// -----------------------------------------------------------------------

void GraphicsSystem::Resize(Size display_size) {
  display_size_ = display_size;
  if (impl_)
    impl_->Resize(display_size_, screen_mode() == 0);
  ForceRefresh();
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetScreenMode(const int in) {
  globals_.screen_mode = in;

  Resize(screen_size());
}

// -----------------------------------------------------------------------

void GraphicsSystem::ToggleFullscreen() {
  SetScreenMode(screen_mode() ? 0 : 1);
}

// -----------------------------------------------------------------------

void GraphicsSystem::ToggleInterfaceHidden() {
  interface_hidden_ = !interface_hidden_;
}

// -----------------------------------------------------------------------

const ObjectSettings& GraphicsSystem::GetObjectSettings(const int obj_num) {
  return graphics_object_settings_->GetObjectSettingsFor(obj_num);
}

// -----------------------------------------------------------------------

void GraphicsSystem::DrawFrame() {
  switch (background_type_) {
    case BACKGROUND_DC0: {
      // Display DC0
      GetDC(0)->RenderToScreen(screen_rect(), screen_rect(), 255);
      break;
    }
    case BACKGROUND_HIK: {
      if (hik_renderer_) {
        hik_renderer_->Render();
      } else {
        GetHaikei()->RenderToScreen(screen_rect(), screen_rect(), 255);
      }
    }
  }

  RenderObjects();

  // Render text
  if (!is_interface_hidden())
    system().text().Render();
}

// -----------------------------------------------------------------------

void GraphicsSystem::RenderFrame(bool should_refresh) {
  RenderFrameConfig config{
      .screen_size = screen_size(),
      .display_size = display_size_,
      .screen_origin = GetScreenOrigin(),
      .manual_update_mode = screen_update_mode_ == SCREENUPDATEMODE_MANUAL};

  auto draw_scene = [this]() { DrawFrame(); };
  auto draw_renderables = [this]() {
    std::erase_if(final_renderers_, [](std::weak_ptr<Renderable> wp) {
      std::shared_ptr<Renderable> renderable = wp.lock();
      if (!renderable)
        return true;
      renderable->Render();
      return false;
    });
  };
  auto draw_cursor = [this]() {
    if (!ShouldUseCustomCursor())
      return;
    std::shared_ptr<MouseCursor> cursor;
    if (system().rlEvent().mouse_inside_window())
      cursor = GetCurrentCursor();
    if (cursor)
      cursor->RenderHotspotAt(cursor_pos());
  };

  if (!should_refresh) {
    impl_->RedrawLastFrame(config, draw_cursor);
    return;
  }

  impl_->RenderFrame(config, draw_scene, draw_renderables, draw_cursor);
}

// -----------------------------------------------------------------------

void GraphicsSystem::RenderCustomFrame(const DrawCallback& draw_scene,
                                       const DrawCallback& draw_after) {
  RenderFrameConfig config{
      .screen_size = screen_size(),
      .display_size = display_size_,
      .screen_origin = GetScreenOrigin(),
      .manual_update_mode = screen_update_mode_ == SCREENUPDATEMODE_MANUAL};

  auto draw_cursor = [this]() {
    if (!ShouldUseCustomCursor())
      return;
    std::shared_ptr<MouseCursor> cursor;
    if (system().rlEvent().mouse_inside_window())
      cursor = GetCurrentCursor();
    if (cursor)
      cursor->RenderHotspotAt(cursor_pos());
  };

  // Preserve the old Begin/EndFrame behavior: render final_renderers_ and then
  // the optional draw_after()
  auto draw_renderables_then_after = [this, &draw_after]() {
    std::erase_if(final_renderers_, [](std::weak_ptr<Renderable> wp) {
      std::shared_ptr<Renderable> renderable = wp.lock();
      if (!renderable)
        return true;
      renderable->Render();
      return false;
    });

    if (draw_after)
      draw_after();
  };

  impl_->RenderFrame(config, draw_scene, draw_renderables_then_after,
                     draw_cursor);
}

// -----------------------------------------------------------------------

std::shared_ptr<SDLSurface> GraphicsSystem::RenderToSurface() {
  RenderFrameConfig config{
      .screen_size = screen_size(),
      .display_size = display_size_,
      .screen_origin = GetScreenOrigin(),
      .manual_update_mode = screen_update_mode_ == SCREENUPDATEMODE_MANUAL};
  auto draw_scene = [this]() { DrawFrame(); };
  return impl_->RenderToSurface(config, draw_scene);
}

// -----------------------------------------------------------------------

void GraphicsSystem::UpdateWindowTitle() {
  if (!impl_)
    return;

  Clock::timepoint_t now = window_title_clock_.GetTime();
  if (now - last_window_title_update_ < window_title_update_interval_)
    return;

  last_window_title_update_ = now;
  std::string title = ComposeWindowTitle();
  impl_->SetWindowTitle(title);
  current_window_title_ = std::move(title);
}

// -----------------------------------------------------------------------

std::string GraphicsSystem::ComposeWindowTitle() const {
  std::string result = caption_title_utf8_;
  if (should_display_subtitle() && !subtitle_utf8_.empty()) {
    if (!result.empty())
      result += ": ";
    result += subtitle_utf8_;
  }
  return result;
}

// -----------------------------------------------------------------------

void GraphicsSystem::ExecuteGraphicsSystem(RLMachine& machine) {
  if (mouse_cursor_)
    mouse_cursor_->Execute();

  // Possibly update the screen shaking state
  if (!screen_shake_queue_.empty()) {
    unsigned int now = system().event().GetTicks();
    unsigned int accumulated_ticks = now - time_at_last_queue_change_;
    while (!screen_shake_queue_.empty() &&
           accumulated_ticks > screen_shake_queue_.front().second) {
      int frame_ticks = screen_shake_queue_.front().second;
      accumulated_ticks -= frame_ticks;
      time_at_last_queue_change_ += frame_ticks;
      screen_shake_queue_.pop();
      ForceRefresh();
    }
  }

  if (is_responsible_for_update()) {
    switch (screen_update_mode_) {
      case SCREENUPDATEMODE_AUTOMATIC:
      case SCREENUPDATEMODE_SEMIAUTOMATIC:
        screen_needs_refresh_ = true;
        break;
      case SCREENUPDATEMODE_MANUAL:
        break;
      default:
        throw std::runtime_error("GraphicsSystem: Invalid screen update mode " +
                                 std::to_string(screen_update_mode_));
    }

    RenderFrame(screen_needs_refresh_);
    screen_needs_refresh_ = false;
  }

  UpdateWindowTitle();
}

// -----------------------------------------------------------------------

void GraphicsSystem::Reset() {
  ClearAllDCs();

  preloaded_hik_scripts_.Clear();
  preloaded_g00_.Clear();
  hik_renderer_.reset();
  background_type_ = BACKGROUND_DC0;

  // Reset the cursor
  show_cursor_from_bytecode_ = true;
  cursor_ = system().gameexe()("MOUSE_CURSOR").Int().value_or(0);
  mouse_cursor_.reset();
  if (impl_)
    impl_->ShowSystemCursor(!ShouldUseCustomCursor());

  screen_update_mode_ = SCREENUPDATEMODE_AUTOMATIC;
  background_type_ = BACKGROUND_DC0;
  subtitle_.clear();
  subtitle_utf8_.clear();
  current_window_title_.clear();
  last_window_title_update_ =
      window_title_clock_.GetTime() - window_title_update_interval_;
  UpdateWindowTitle();
  interface_hidden_ = false;
}

std::shared_ptr<SDLSurface> GraphicsSystem::GetEmojiSurface() {
  for (auto it : system().gameexe().Filter("E_MOJI.")) {
    // Try to interpret each key as a filename.
    std::string file_name = it.Str().value_or("");
    std::shared_ptr<SDLSurface> surface = GetSurfaceNamed(file_name);
    if (surface)
      return surface;
  }

  return nullptr;
}

void GraphicsSystem::PreloadHIKScript(System& system,
                                      int slot,
                                      const std::string& name,
                                      const std::filesystem::path& file_path) {
  auto script = std::make_shared<HIKScript>(system, file_path);
  preloaded_hik_scripts_[slot] = std::make_pair(name, script);
}

void GraphicsSystem::ClearPreloadedHIKScript(int slot) {
  preloaded_hik_scripts_[slot] = std::make_pair("", nullptr);
}

void GraphicsSystem::ClearAllPreloadedHIKScripts() {
  preloaded_hik_scripts_.Clear();
}

std::shared_ptr<HIKScript> GraphicsSystem::GetHIKScript(
    System& system,
    const std::string& name,
    const std::filesystem::path& file_path) {
  for (HIKArrayItem& item : preloaded_hik_scripts_) {
    if (item.first == name)
      return item.second;
  }

  return std::make_shared<HIKScript>(system, file_path);
}

void GraphicsSystem::PreloadG00(int slot, const std::string& name) {
  // We first check our implicit cache just in case so we don't load it twice.
  std::shared_ptr<SDLSurface> surface = image_cache_.fetch(name);
  if (!surface)
    surface = LoadSurfaceFromFile(name);

  preloaded_g00_[slot] = std::make_pair(name, surface);
}

void GraphicsSystem::ClearPreloadedG00(int slot) {
  preloaded_g00_[slot] = std::make_pair("", nullptr);
}

void GraphicsSystem::ClearAllPreloadedG00() { preloaded_g00_.Clear(); }

std::shared_ptr<SDLSurface> GraphicsSystem::GetPreloadedG00(
    const std::string& name) {
  for (G00ArrayItem& item : preloaded_g00_) {
    if (item.first == name)
      return item.second;
  }

  return nullptr;
}

// -----------------------------------------------------------------------

std::shared_ptr<SDLSurface> GraphicsSystem::LoadSurfaceFromFile(
    const std::string& short_filename) {
  static const std::set<std::string> IMAGE_FILETYPES = {"g00", "pdt"};
  auto pth = asset_scanner_->FindFile(short_filename, IMAGE_FILETYPES);

  if (!pth.has_value())
    throw pth.error();

  if (pth->empty())
    throw std::runtime_error(
        std::format("Could not load image file '{}'", short_filename));

  std::shared_ptr<SDLSurface> result = impl_->LoadSurface(*pth);
  // handle tone curve effect loading
  if (auto pos = short_filename.find("?"); pos != short_filename.npos) {
    auto effect_str = std::string_view(short_filename).substr(pos + 1);
    int effect_no = 0;
    if (auto [ptr, ec] =
            std::from_chars(effect_str.begin(), effect_str.end(), effect_no);
        ec != std::errc{} || ptr != effect_str.end()) {
      throw std::runtime_error(
          std::format("Invalid tone curve query '{}'", effect_str));
    }

    // the effect number is an index that goes from 10 to GetEffectCount() * 10,
    // so keep that in mind here
    if ((effect_no / 10) > globals().tone_curves.GetEffectCount() ||
        effect_no < 10)
      throw std::runtime_error(
          std::format("Tone curve index {} is invalid.", effect_no));

    result->Apply([effect = globals().tone_curves.GetEffect(effect_no / 10 -
                                                            1)](RGBAColour c) {
      c.set_red(effect[0][c.r()]);
      c.set_green(effect[1][c.g()]);
      c.set_blue(effect[2][c.b()]);
      return c;
    });
  }

  return result;
}

// -----------------------------------------------------------------------

std::shared_ptr<SDLSurface> GraphicsSystem::GetSurfaceNamedAndMarkViewed(
    RLMachine& machine,
    const std::string& short_filename) {
  // Record that we viewed this CG.
  cg_table().SetViewed(short_filename);

  // Set the intZ[] flag
  int flag = cg_table().GetFlag(short_filename);
  if (flag != -1) {
    machine.GetMemory().Write(
        libreallive::IntMemRef(libreallive::INTZ_LOCATION, 0, flag), 1);
  }

  return GetSurfaceNamed(short_filename);
}

// -----------------------------------------------------------------------

std::shared_ptr<SDLSurface> GraphicsSystem::GetSurfaceNamed(
    const std::string& short_filename) {
  // Check if this is in the script controlled cache.
  std::shared_ptr<SDLSurface> cached_surface = GetPreloadedG00(short_filename);
  if (cached_surface)
    return cached_surface;

  // First check to see if this surface is already in our internal cache
  cached_surface = image_cache_.fetch(short_filename);
  if (cached_surface)
    return cached_surface;

  std::shared_ptr<SDLSurface> surface_to_ret =
      LoadSurfaceFromFile(short_filename);
  image_cache_.insert(short_filename, surface_to_ret);
  return surface_to_ret;
}

// -----------------------------------------------------------------------

int GraphicsSystem::GetObjectLayerSize() {
  return graphics_object_settings_->objects_in_a_layer;
}

// -----------------------------------------------------------------------

std::shared_ptr<SDLSurface> GraphicsSystem::GetHaikei() {
  if (haikei_->RawSurface() == NULL) {
    haikei_->Allocate(screen_size());
  }

  return haikei_;
}

// -----------------------------------------------------------------------

void GraphicsSystem::AllocateDC(int dc, Size size) {
  if (dc < 0 || dc >= 16)
    throw std::runtime_error(std::format(
        "Invalid DC number '{}' in GraphicsSystem::allocate_dc", dc));

  // We can't reallocate the screen!
  if (dc == 0)
    throw std::runtime_error("Attempting to reallocate DC 0!");

  // DC 1 is a special case and must always be at least the size of
  // the screen.
  if (dc == 1) {
    Size dc0_size = display_contexts_[0]->GetSize();
    if (size.width() < dc0_size.width())
      size.set_width(dc0_size.width());
    if (size.height() < dc0_size.height())
      size.set_height(dc0_size.height());
  }

  // Allocate a new obj.
  display_contexts_[dc]->Allocate(size);
}

void GraphicsSystem::SetMinimumSizeForDC(int dc, Size size) {
  if (display_contexts_[dc] == NULL || !display_contexts_[dc]->IsAllocated()) {
    AllocateDC(dc, size);
  } else {
    Size current = display_contexts_[dc]->GetSize();
    if (current.width() < size.width() || current.height() < size.height()) {
      // Make a new surface of the maximum size.
      Size maxSize = current.SizeUnion(size);

      std::shared_ptr<SDLSurface> newdc = std::make_shared<SDLSurface>();
      newdc->Allocate(maxSize);

      display_contexts_[dc]->BlitToSurface(*newdc,
                                           display_contexts_[dc]->GetRect(),
                                           display_contexts_[dc]->GetRect());

      display_contexts_[dc] = newdc;
    }
  }
}

void GraphicsSystem::FreeDC(int dc) {
  if (dc == 0) {
    throw std::runtime_error("Attempt to deallocate DC[0]");
  } else if (dc == 1) {
    // DC[1] never gets freed; it only gets blanked
    GetDC(1)->Fill(RGBAColour::Black());
  } else {
    display_contexts_[dc]->Deallocate();
  }
}

std::shared_ptr<SDLSurface> GraphicsSystem::GetDC(int dc) {
  assert(0 <= dc && dc < 16);

  // If requesting a DC that doesn't exist, allocate it first.
  if (display_contexts_[dc]->RawSurface() == NULL)
    AllocateDC(dc, display_contexts_[0]->GetSize());

  return display_contexts_[dc];
}

void GraphicsSystem::ClearAllDCs() {
  GetDC(0)->Fill(RGBAColour::Black());

  for (int i = 1; i < 16; ++i)
    FreeDC(i);
}

// -----------------------------------------------------------------------

void GraphicsSystem::RenderObjects() {
  BoundStage().RenderObjects(
      [this](size_t obj_number, const GraphicsObject&) -> bool {
        const ObjectSettings& settings =
            GetObjectSettings(static_cast<int>(obj_number));
        if (settings.obj_on_off == 1 && should_show_object1() == false)
          return false;
        else if (settings.obj_on_off == 2 && should_show_object2() == false)
          return false;
        else if (settings.weather_on_off && should_show_weather() == false)
          return false;
        else if (settings.space_key && is_interface_hidden())
          return false;

        return true;
      });
}

// -----------------------------------------------------------------------

std::shared_ptr<MouseCursor> GraphicsSystem::GetCurrentCursor() {
  if (!use_custom_mouse_cursor_ || !show_cursor_from_bytecode_)
    return std::shared_ptr<MouseCursor>();

  if (use_custom_mouse_cursor_ && !mouse_cursor_) {
    MouseCursorCache::iterator it = cursor_cache_.find(cursor_);
    if (it != cursor_cache_.end()) {
      mouse_cursor_ = it->second;
    } else {
      std::shared_ptr<SDLSurface> cursor_surface;
      GameexeInterpretObject cursor =
          system().gameexe()("MOUSE_CURSOR", cursor_);

      if (auto name = cursor("NAME").Str()) {
        int count = cursor("CONT").Int().value_or(1);
        int speed = cursor("SPEED").Int().value_or(800);

        cursor_surface = GetSurfaceNamed(*name);
        mouse_cursor_ = std::make_shared<MouseCursor>(
            system().event().GetClock(), cursor_surface, count, speed);
        cursor_cache_[cursor_] = mouse_cursor_;
      } else {
        mouse_cursor_.reset();
      }
    }
  }

  return mouse_cursor_;
}

// -----------------------------------------------------------------------

void GraphicsSystem::SetScreenSize(const Size& size) {
  screen_size_ = size;
  screen_rect_ = Rect(Point(0, 0), size);
}

// -----------------------------------------------------------------------

void GraphicsSystem::OnEvent(std::shared_ptr<Event> event) {
  std::visit(
      [&](const auto& event) {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::same_as<T, MouseMotion>)
          cursor_pos_ = event.pos;
      },
      *event);
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsSystem::save(Archive& ar, unsigned int version) const {
  Stage& stage = BoundStage();
  ar & subtitle_ & stage.saved_graphics_stack & stage.saved_background_objects &
      stage.saved_foreground_objects;
}

// -----------------------------------------------------------------------

template <class Archive>
void GraphicsSystem::load(Archive& ar, unsigned int version) {
  Stage& stage = BoundStage();

  ar & subtitle_;
  if (version > 0) {
    ar & stage.graphics_stack;
  } else {
    throw std::runtime_error("Deprecated old graphics stack has been removed");
  }

  ar & stage.background_objects & stage.foreground_objects;

  // Now alert all subclasses that we've set the subtitle
  SetWindowSubtitle(subtitle_,
                    Serialization::g_current_machine->GetTextEncoding());
}

// -----------------------------------------------------------------------

template void GraphicsSystem::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
template void GraphicsSystem::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;
