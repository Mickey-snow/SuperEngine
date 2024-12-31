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

#include "GL/glew.h"

#include "systems/sdl/sdl_graphics_system.hpp"

#include <SDL/SDL.h>

#if !defined(__APPLE__) && !defined(_WIN32)
#include <SDL/SDL_image.h>
#include "../resources/48/rlvm_icon_48.xpm"
#endif

#include "base/asset_scanner.hpp"
#include "base/avdec/image_decoder.hpp"
#include "base/cgm_table.hpp"
#include "base/colour.hpp"
#include "base/gameexe.hpp"
#include "base/notification/source.hpp"
#include "base/tone_curve.hpp"
#include "machine/rlmachine.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/mouse_cursor.hpp"
#include "systems/base/renderable.hpp"
#include "systems/base/system.hpp"
#include "systems/base/system_error.hpp"
#include "systems/base/text_system.hpp"
#include "systems/glcanvas.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"
#include "systems/screen_canvas.hpp"
#include "systems/sdl/sdl_event_system.hpp"
#include "systems/sdl/sdl_utils.hpp"
#include "systems/sdl/shaders.hpp"
#include "systems/sdl_surface.hpp"
#include "utilities/exception.hpp"
#include "utilities/graphics.hpp"
#include "utilities/lazy_array.hpp"
#include "utilities/mapped_file.hpp"
#include "utilities/string_utilities.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

void SDLGraphicsSystem::SetCursor(int cursor) {
  GraphicsSystem::SetCursor(cursor);

  SDL_ShowCursor(ShouldUseCustomCursor() ? SDL_DISABLE : SDL_ENABLE);
}

std::shared_ptr<glCanvas> SDLGraphicsSystem::CreateCanvas() const {
  return std::make_shared<glCanvas>(screen_size(), display_size_,
                                    GetScreenOrigin());
}

void SDLGraphicsSystem::BeginFrame() {
  glRenderer renderer;
  renderer.SetUp();
  renderer.ClearBuffer(std::make_shared<ScreenCanvas>(screen_size()),
                       RGBAColour(0, 0, 0, 255));
  DebugShowGLErrors();

  glViewport(0, 0, display_size_.width(), display_size_.height());

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, (GLdouble)display_size_.width(),
          (GLdouble)display_size_.height(), 0.0, 0.0, 1.0);
  DebugShowGLErrors();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  DebugShowGLErrors();

  // Full screen shaking moves where the origin is.
  Point origin = GetScreenOrigin();
  auto aspect_ratio_w =
      static_cast<float>(display_size_.width()) / screen_size().width();
  auto aspect_ratio_h =
      static_cast<float>(display_size_.height()) / screen_size().height();
  glTranslatef(origin.x() * aspect_ratio_w, origin.y() * aspect_ratio_h, 0);
}

void SDLGraphicsSystem::EndFrame() {
  FinalRenderers::iterator it = renderer_begin();
  FinalRenderers::iterator end = renderer_end();
  for (; it != end; ++it) {
    (*it)->Render();
  }

  if (screen_update_mode() == SCREENUPDATEMODE_MANUAL) {
    if (!screen_contents_texture_) {
      screen_contents_texture_ = std::make_shared<glTexture>(display_size_);
    }

    // Copy the area behind the cursor to the temporary buffer (drivers differ:
    // the contents of the back buffer is undefined after SDL_GL_SwapBuffers()
    // and I've just been lucky that the Intel i810 and whatever my Mac machine
    // has have been doing things that way.)
    glBindTexture(GL_TEXTURE_2D, screen_contents_texture_->GetID());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, display_size_.width(),
                        display_size_.height());
    screen_contents_texture_valid_ = true;
  } else {
    screen_contents_texture_valid_ = false;
  }

  DrawCursor();

  // Swap the buffers
  glFlush();
  SDL_GL_SwapBuffers();
  ShowGLErrors();
}

void SDLGraphicsSystem::RedrawLastFrame() {
  // We won't redraw the screen between when the DrawManual() command is issued
  // by the bytecode and the first refresh() is called since we need a valid
  // copy of the screen to work with and we only snapshot the screen during
  // DrawManual() mode.
  if (screen_contents_texture_valid_ && screen_contents_texture_) {
    glRenderer renderer;

    renderer.Render(
        {screen_contents_texture_, Rect(Point(0, 0), display_size_)},
        {std::make_shared<ScreenCanvas>(screen_size()),
         Rect(Point(0, 0), screen_size())});

    DrawCursor();

    // Swap the buffers
    SDL_GL_SwapBuffers();
    ShowGLErrors();
  }
}

std::shared_ptr<Surface> SDLGraphicsSystem::RenderToSurface() {
  auto canvas = CreateCanvas();
  canvas->Use();

  const auto original_screen = SDLSurface::screen_;
  SDLSurface::screen_ = canvas->GetBuffer();
  DrawFrame();  // the actual drawing
  SDLSurface::screen_ = original_screen;

  // read back pixels
  auto texture = canvas->GetBuffer()->GetTexture();
  const auto width = screen_size().width();
  const auto height = screen_size().height();

  std::vector<GLubyte> buf(width * height * 4);
  glGetTextureSubImage(texture->GetID(), 0, 0, 0, 0, width, height, 1, GL_RGBA,
                       GL_UNSIGNED_BYTE, buf.size(), buf.data());

  GLubyte* pixels = new GLubyte[buf.size()];
  for (int y = 0; y < height; ++y) {
    std::memcpy(pixels + (y * width * 4),
                buf.data() + ((height - 1 - y) * width * 4), width * 4);
  }

  SDL_Surface* surface =
      SDL_CreateRGBSurfaceFrom(pixels, width, height, 32, width * 4, 0xFF000000,
                               0x00FF0000, 0x0000FF00, 0x000000FF);

  return std::make_shared<SDLSurface>(surface);
}

void SDLGraphicsSystem::DrawCursor() {
  if (ShouldUseCustomCursor()) {
    std::shared_ptr<MouseCursor> cursor;
    if (static_cast<SDLEventSystem&>(system().event()).mouse_inside_window())
      cursor = GetCurrentCursor();
    if (cursor) {
      Point hotspot = cursor_pos();
      cursor->RenderHotspotAt(hotspot);
    }
  }
}

SDLGraphicsSystem::SDLGraphicsSystem(System& system, Gameexe& gameexe)
    : GraphicsSystem(system, gameexe),
      redraw_last_frame_(false),
      screen_contents_texture_valid_(false),
      asset_scanner_(system.GetAssetScanner()) {
  haikei_ = std::make_shared<SDLSurface>();
  for (int i = 0; i < 16; ++i)
    display_contexts_[i] = std::make_shared<SDLSurface>();

  // Grab the caption
  std::string cp932caption = gameexe("CAPTION").ToString();
  int name_enc = gameexe("NAME_ENC").ToInt(0);
  caption_title_ = cp932toUTF8(cp932caption, name_enc);

  SetupVideo(GetScreenSize(gameexe));

  // Now we allocate the first two display contexts with equal size to
  // the display
  display_contexts_[0]->allocate(screen_size());
  display_contexts_[0]->RegisterObserver(
      [this]() { this->MarkScreenAsDirty(GUT_DRAW_DC0); });
  display_contexts_[1]->allocate(screen_size());

  SetWindowTitle(caption_title_);

#if !defined(__APPLE__) && !defined(_WIN32)
  // We only set the icon on Linux because OSX will use the icns file
  // automatically and this doesn't look too awesome.
  SDL_Surface* icon = IMG_ReadXPMFromArray(rlvm_icon_48);
  if (icon) {
    SDL_SetColorKey(icon, SDL_SRCCOLORKEY,
                    SDL_MapRGB(icon->format, 255, 255, 255));
    SDL_WM_SetIcon(icon, NULL);
    SDL_FreeSurface(icon);
  }
#endif

  SDL_ShowCursor(ShouldUseCustomCursor() ? SDL_DISABLE : SDL_ENABLE);

  registrar_.Add(this, NotificationType::FULLSCREEN_STATE_CHANGED,
                 Source<GraphicsSystem>(static_cast<GraphicsSystem*>(this)));
}

void SDLGraphicsSystem::Resize(Size display_size) {
  if (auto fake_screen =
          std::dynamic_pointer_cast<ScreenCanvas>(SDLSurface::screen_)) {
    fake_screen->display_size_ = display_size;
  }
  display_size_ = display_size;
  screen_contents_texture_ = nullptr;

  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  if (!info) {
    std::ostringstream ss;
    ss << "Video query failed: " << SDL_GetError();
    throw std::runtime_error(ss.str());
  }

  int bpp = info->vfmt->BitsPerPixel;

  // the flags to pass to SDL_SetVideoMode
  int video_flags;
  video_flags = SDL_OPENGL;            // Enable OpenGL in SDL
  video_flags |= SDL_GL_DOUBLEBUFFER;  // Enable double buffering
  video_flags |= SDL_SWSURFACE;
  video_flags |= SDL_RESIZABLE;

  if (screen_mode() == 0)
    video_flags |= SDL_FULLSCREEN;

  // Sets up OpenGL double buffering
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Set the video mode
  if ((screen_ = SDL_SetVideoMode(display_size_.width(), display_size_.height(),
                                  bpp, video_flags)) == 0) {
    std::ostringstream ss;
    ss << "Video mode set failed: " << SDL_GetError();
    throw std::runtime_error(ss.str());
  }
}

void SDLGraphicsSystem::SetupVideo(Size window_size) {
  GraphicsSystem::SetScreenSize(window_size);
  SDLSurface::screen_ = std::make_shared<ScreenCanvas>(screen_size());

  Resize(window_size);

  // Initialize glew
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    std::ostringstream oss;
    oss << "Failed to initialize GLEW: " << glewGetErrorString(err);
    throw SystemError(oss.str());
  }

  ShowGLErrors();
}

SDLGraphicsSystem::~SDLGraphicsSystem() {}

void SDLGraphicsSystem::ExecuteGraphicsSystem(RLMachine& machine) {
  // For now, nothing, but later, we need to put all code each cycle
  // here.
  if (is_responsible_for_update() && screen_needs_refresh()) {
    BeginFrame();
    DrawFrame();
    EndFrame();
    screen_needs_refresh_ = false;
    object_state_dirty_ = false;
    redraw_last_frame_ = false;
  } else if (is_responsible_for_update() && redraw_last_frame_) {
    RedrawLastFrame();
    redraw_last_frame_ = false;
  }

  // Update the seen.
  static auto clock = Clock();
  static auto time_of_last_titlebar_update = clock.GetTime();
  auto current_time = clock.GetTime();
  if ((current_time - time_of_last_titlebar_update) >
      std::chrono::milliseconds(60)) {
    time_of_last_titlebar_update = current_time;
    std::string new_caption = caption_title_;
    if (should_display_subtitle() && subtitle_ != "") {
      new_caption += ": " + subtitle_;
    }

    SetWindowTitle(std::move(new_caption));
  }

  GraphicsSystem::ExecuteGraphicsSystem(machine);
}

void SDLGraphicsSystem::MarkScreenAsDirty(GraphicsUpdateType type) {
  if (is_responsible_for_update() &&
      screen_update_mode() == SCREENUPDATEMODE_MANUAL &&
      type == GUT_MOUSE_MOTION)
    redraw_last_frame_ = true;
  else
    GraphicsSystem::MarkScreenAsDirty(type);
}

void SDLGraphicsSystem::SetWindowTitle(std::string new_caption) {
  // PulseAudio allocates a string each time we set the title. Make sure we
  // don't do this unnecessarily.
  static std::string currently_set_title_ = "???";
  if (new_caption != currently_set_title_) {
    SDL_WM_SetCaption(new_caption.c_str(), NULL);
    currently_set_title_ = new_caption;
  }
}

void SDLGraphicsSystem::Observe(NotificationType type,
                                const NotificationSource& source,
                                const NotificationDetails& details) {}

void SDLGraphicsSystem::SetWindowSubtitle(const std::string& cp932str,
                                          int text_encoding) {
  // TODO(erg): Still not restoring title correctly!
  subtitle_ = cp932toUTF8(cp932str, text_encoding);

  GraphicsSystem::SetWindowSubtitle(cp932str, text_encoding);
}

void SDLGraphicsSystem::SetScreenMode(const int in) {
  GraphicsSystem::SetScreenMode(in);

  SetupVideo(screen_size());
}

void SDLGraphicsSystem::AllocateDC(int dc, Size size) {
  if (dc >= 16) {
    std::ostringstream ss;
    ss << "Invalid DC number \"" << dc
       << "\" in SDLGraphicsSystem::allocate_dc";
    throw rlvm::Exception(ss.str());
  }

  // We can't reallocate the screen!
  if (dc == 0)
    throw rlvm::Exception("Attempting to reallocate DC 0!");

  // DC 1 is a special case and must always be at least the size of
  // the screen.
  if (dc == 1) {
    SDL_Surface* dc0 = display_contexts_[0]->rawSurface();
    if (size.width() < dc0->w)
      size.set_width(dc0->w);
    if (size.height() < dc0->h)
      size.set_height(dc0->h);
  }

  // Allocate a new obj.
  display_contexts_[dc]->allocate(size);
}

void SDLGraphicsSystem::SetMinimumSizeForDC(int dc, Size size) {
  if (display_contexts_[dc] == NULL || !display_contexts_[dc]->allocated()) {
    AllocateDC(dc, size);
  } else {
    Size current = display_contexts_[dc]->GetSize();
    if (current.width() < size.width() || current.height() < size.height()) {
      // Make a new surface of the maximum size.
      Size maxSize = current.SizeUnion(size);

      std::shared_ptr<SDLSurface> newdc = std::make_shared<SDLSurface>();
      newdc->allocate(maxSize);

      display_contexts_[dc]->BlitToSurface(*newdc,
                                           display_contexts_[dc]->GetRect(),
                                           display_contexts_[dc]->GetRect());

      display_contexts_[dc] = newdc;
    }
  }
}

void SDLGraphicsSystem::FreeDC(int dc) {
  if (dc == 0) {
    throw rlvm::Exception("Attempt to deallocate DC[0]");
  } else if (dc == 1) {
    // DC[1] never gets freed; it only gets blanked
    GetDC(1)->Fill(RGBAColour::Black());
  } else {
    display_contexts_[dc]->deallocate();
  }
}

void SDLGraphicsSystem::VerifySurfaceExists(int dc, const std::string& caller) {
  if (dc >= 16) {
    std::ostringstream ss;
    ss << "Invalid DC number (" << dc << ") in " << caller;
    throw rlvm::Exception(ss.str());
  }

  if (display_contexts_[dc] == NULL) {
    std::ostringstream ss;
    ss << "Parameter DC[" << dc << "] not allocated in " << caller;
    throw rlvm::Exception(ss.str());
  }
}

// -----------------------------------------------------------------------

typedef enum { NO_MASK, ALPHA_MASK, COLOR_MASK } MaskType;
static SDL_Surface* newSurfaceFromRGBAData(int w,
                                           int h,
                                           char* data,
                                           MaskType with_mask) {
  // Note to self: These describe the byte order IN THE RAW G00 DATA!
  // These should NOT be switched to native byte order.
  constexpr auto DefaultBpp = 32;
  constexpr auto DefaultAmask = 0xff000000;
  constexpr auto DefaultRmask = 0xff0000;
  constexpr auto DefaultGmask = 0xff00;
  constexpr auto DefaultBmask = 0xff;

  int amask = (with_mask == ALPHA_MASK) ? DefaultAmask : 0;
  SDL_Surface* tmp =
      SDL_CreateRGBSurfaceFrom(data, w, h, DefaultBpp, w * 4, DefaultRmask,
                               DefaultGmask, DefaultBmask, amask);

  // We now need to convert this surface to a format suitable for use across
  // the rest of the program. We can't (regretfully) rely on
  // SDL_DisplayFormat[Alpha] to decide on a format that we can send to OpenGL
  // (see some Intel macs) so use convert surface to a pixel order our data
  // correctly while still using the appropriate alpha flags. So use the above
  // format with only the flags that would have been set by
  // SDL_DisplayFormat[Alpha].
  Uint32 flags;
  if (with_mask == ALPHA_MASK) {
    flags = tmp->flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
  } else {
    flags = tmp->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA | SDL_RLEACCELOK);
  }

  SDL_Surface* surf = SDL_ConvertSurface(tmp, tmp->format, flags);
  SDL_FreeSurface(tmp);
  return surf;
}

std::shared_ptr<SDLSurface> GetSDLSurface(std::shared_ptr<Surface> surface) {
  if (auto sdl_surface = std::dynamic_pointer_cast<SDLSurface>(surface))
    return sdl_surface;
  throw std::runtime_error("SDLGraphicsSystem: expected sdl surface.");
}

std::shared_ptr<const Surface> SDLGraphicsSystem::LoadSurfaceFromFile(
    const std::string& short_filename) {
  static const std::set<std::string> IMAGE_FILETYPES = {"g00", "pdt"};
  std::filesystem::path filename =
      asset_scanner_->FindFile(short_filename, IMAGE_FILETYPES);

  if (filename.empty()) {
    std::ostringstream oss;
    oss << "Could not find image file \"" << short_filename << "\".";
    throw rlvm::Exception(oss.str());
  }

  MappedFile file(filename);
  ImageDecoder dec(file.Read());

  const auto width = dec.width;
  const auto height = dec.height;

  // do not free until SDL_FreeSurface() is called on the surface using it
  char* mem = dec.mem.data();
  SDL_Surface* s = nullptr;
  MaskType is_mask = dec.ismask ? ALPHA_MASK : NO_MASK;
  if (is_mask == ALPHA_MASK) {
    int len = width * height;
    uint32_t* d = reinterpret_cast<uint32_t*>(mem);
    int i;
    for (i = 0; i < len; i++) {
      if ((*d & 0xff000000) != 0xff000000)
        break;
      d++;
    }
    if (i == len) {
      is_mask = NO_MASK;
    }
  }

  s = newSurfaceFromRGBAData(width, height, mem, is_mask);

  // Grab the Type-2 information out of the converter or create one
  // default region if none exist
  std::vector<GrpRect> region_table = dec.region_table;
  if (region_table.empty()) {
    GrpRect rect;
    rect.rect = Rect(Point(0, 0), Size(width, height));
    rect.originX = 0;
    rect.originY = 0;
    region_table.push_back(rect);
  }

  std::shared_ptr<Surface> surface_to_ret =
      std::make_shared<SDLSurface>(s, region_table);

  // handle tone curve effect loading
  if (short_filename.find("?") != short_filename.npos) {
    std::string effect_no_str =
        short_filename.substr(short_filename.find("?") + 1);
    int effect_no = std::stoi(effect_no_str);
    // the effect number is an index that goes from 10 to GetEffectCount() * 10,
    // so keep that in mind here
    if ((effect_no / 10) > globals().tone_curves.GetEffectCount() ||
        effect_no < 10) {
      std::ostringstream oss;
      oss << "Tone curve index " << effect_no << " is invalid.";
      throw rlvm::Exception(oss.str());
    }
    surface_to_ret.get()->ToneCurve(
        globals().tone_curves.GetEffect(effect_no / 10 - 1),
        Rect(Point(0, 0), Size(width, height)));
  }

  return surface_to_ret;
}

std::shared_ptr<Surface> SDLGraphicsSystem::GetHaikei() {
  if (haikei_->rawSurface() == NULL) {
    haikei_->allocate(screen_size());
    haikei_->RegisterObserver(
        [this]() { this->MarkScreenAsDirty(GUT_DRAW_DC0); });
  }

  return haikei_;
}

std::shared_ptr<Surface> SDLGraphicsSystem::GetDC(int dc) {
  VerifySurfaceExists(dc, "SDLGraphicsSystem::get_dc");

  // If requesting a DC that doesn't exist, allocate it first.
  if (display_contexts_[dc]->rawSurface() == NULL)
    AllocateDC(dc, display_contexts_[0]->GetSize());

  return display_contexts_[dc];
}

void SDLGraphicsSystem::Reset() { GraphicsSystem::Reset(); }

Size SDLGraphicsSystem::GetDisplaySize() const noexcept {
  return display_size_;
}
