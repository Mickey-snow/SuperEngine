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
//
// -----------------------------------------------------------------------

#include "GL/glew.h"

#include "systems/sdl/graphics_backend.hpp"

#include "core/album.hpp"
#include "core/avdec/image_decoder.hpp"
#include "core/colour.hpp"
#include "log/domain_logger.hpp"
#include "systems/gl_utils.hpp"
#include "systems/glcanvas.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"
#include "systems/screen_canvas.hpp"
#include "systems/sdl_surface.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#if !defined(__APPLE__) && !defined(_WIN32)
#include <SDL/SDL_image.h>
#include "../resources/48/rlvm_icon_48.xpm"
#endif

#include <fstream>
#include <string>
#include <vector>

#include <cstring>

using std::string_literals::operator""s;

static DomainLogger logger("SDLGraphicsBackend");

static std::string LoadFile(const std::filesystem::path& pth) {
  std::ifstream ifs(pth, std::ios::binary);
  if (!ifs) {
    logger(Severity::Error) << "Cannot open file: " << pth.string();
    return {};
  }

  return std::string(std::istreambuf_iterator<char>(ifs),
                     std::istreambuf_iterator<char>());
}

SDLGraphicsBackend::SDLGraphicsBackend()
    : screen_(nullptr),
      screen_contents_texture_(nullptr),
      screen_contents_texture_valid_(false) {}

void SDLGraphicsBackend::InitSystem(Size screen_size, bool is_fullscreen) {
  SDLSurface::screen_ = std::make_shared<ScreenCanvas>(screen_size);
  Resize(screen_size, is_fullscreen);

  // Initialize glew
  if (glewInit() != GLEW_OK)
    throw std::runtime_error("Failed to initialize GLEW: " + GetGLErrors());

  ShowGLErrors();

#if !defined(__APPLE__) && !defined(_WIN32)
  SDL_Surface* icon = IMG_ReadXPMFromArray(rlvm_icon_48);
  if (icon) {
    SDL_SetColorKey(icon, SDL_SRCCOLORKEY,
                    SDL_MapRGB(icon->format, 255, 255, 255));
    SDL_WM_SetIcon(icon, NULL);
    SDL_FreeSurface(icon);
  }
#endif
}
void SDLGraphicsBackend::QuitSystem() {}

void SDLGraphicsBackend::Resize(Size display_size, bool is_fullscreen) {
  if (auto fake_screen =
          std::dynamic_pointer_cast<ScreenCanvas>(SDLSurface::screen_)) {
    fake_screen->display_size_ = display_size;
  }

  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  if (!info)
    throw std::runtime_error("Video query failed: "s + SDL_GetError());

  int bpp = info->vfmt->BitsPerPixel;

  // the flags to pass to SDL_SetVideoMode
  int video_flags;
  video_flags = SDL_OPENGL;            // Enable OpenGL in SDL
  video_flags |= SDL_GL_DOUBLEBUFFER;  // Enable double buffering
  video_flags |= SDL_SWSURFACE;
  video_flags |= SDL_RESIZABLE;

  if (is_fullscreen)
    video_flags |= SDL_FULLSCREEN;

  // Sets up OpenGL double buffering
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Set the video mode
  if ((screen_ = SDL_SetVideoMode(display_size.width(), display_size.height(),
                                  bpp, video_flags)) == 0) {
    throw std::runtime_error("Video mode set failed: "s + SDL_GetError());
  }

  screen_contents_texture_.reset();
  screen_contents_texture_valid_ = false;
}

// -----------------------------------------------------------------------

std::shared_ptr<Surface> SDLGraphicsBackend::CreateSurface(Size size) {
  return std::make_shared<SDLSurface>(size);
}

std::shared_ptr<Surface> SDLGraphicsBackend::CreateSurfaceBGRA(
    Size size,
    std::span<char> bgra,
    bool is_alpha_mask) {
  // Note to self: These describe the byte order IN THE RAW G00 DATA!
  // These should NOT be switched to native byte order.
  constexpr auto DefaultBpp = 32;
  constexpr auto DefaultAmask = 0xff000000;
  constexpr auto DefaultRmask = 0xff0000;
  constexpr auto DefaultGmask = 0xff00;
  constexpr auto DefaultBmask = 0xff;

  int amask = is_alpha_mask ? DefaultAmask : 0;
  SDL_Surface* tmp = SDL_CreateRGBSurfaceFrom(
      bgra.data(), size.width(), size.height(), DefaultBpp, size.width() * 4,
      DefaultRmask, DefaultGmask, DefaultBmask, amask);

  // We now need to convert this surface to a format suitable for use across
  // the rest of the program. We can't (regretfully) rely on
  // SDL_DisplayFormat[Alpha] to decide on a format that we can send to OpenGL
  // (see some Intel macs) so use convert surface to a pixel order our data
  // correctly while still using the appropriate alpha flags. So use the above
  // format with only the flags that would have been set by
  // SDL_DisplayFormat[Alpha].
  Uint32 flags;
  if (is_alpha_mask) {
    flags = tmp->flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
  } else {
    flags = tmp->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA | SDL_RLEACCELOK);
  }

  SDL_Surface* surf = SDL_ConvertSurface(tmp, tmp->format, flags);
  SDL_FreeSurface(tmp);

  return std::make_shared<Surface>(surf);
}

std::shared_ptr<Surface> SDLGraphicsBackend::LoadSurface(
    const std::filesystem::path& pth) {
  std::string raw = LoadFile(pth);
  ImageDecoder dec(raw);

  const auto width = dec.width;
  const auto height = dec.height;

  // do not free until SDL_FreeSurface() is called on the surface using it
  char* mem = dec.mem.data();
  bool is_mask = dec.ismask;
  if (is_mask) {
    int len = width * height;
    uint32_t* d = reinterpret_cast<uint32_t*>(mem);
    int i;
    for (i = 0; i < len; i++) {
      if ((*d & 0xff000000) != 0xff000000)
        break;
      d++;
    }
    if (i == len) {
      is_mask = false;
    }
  }

  std::shared_ptr<SDLSurface> s =
      CreateSurfaceBGRA(Size(width, height), dec.mem, is_mask);

  return std::make_shared<SDLSurface>(s->Release(),
                                      std::move(dec.region_table));
}

std::shared_ptr<Album> SDLGraphicsBackend::LoadAlbum(
    const std::filesystem::path& path) {
  std::string raw = LoadFile(path);
  ImageDecoder dec(raw);

  const auto width = dec.width;
  const auto height = dec.height;

  // do not free until SDL_FreeSurface() is called on the surface using it
  char* mem = dec.mem.data();
  bool is_mask = dec.ismask;
  if (is_mask) {
    int len = width * height;
    uint32_t* d = reinterpret_cast<uint32_t*>(mem);
    int i;
    for (i = 0; i < len; i++) {
      if ((*d & 0xff000000) != 0xff000000)
        break;
      d++;
    }
    if (i == len) {
      is_mask = false;
    }
  }

  std::shared_ptr<SDLSurface> s =
      CreateSurfaceBGRA(Size(width, height), dec.mem, is_mask);

  return std::make_shared<Album>(s, std::move(dec.region_table));
}

void SDLGraphicsBackend::SetWindowTitle(const std::string& title_utf8) {
  if (title_utf8 == current_window_title_)
    return;

  SDL_WM_SetCaption(title_utf8.c_str(), nullptr);
  current_window_title_ = title_utf8;
}

void SDLGraphicsBackend::ShowSystemCursor(bool show) {
  SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void SDLGraphicsBackend::RenderFrame(const RenderFrameConfig& config,
                                     const DrawCallback& draw_scene,
                                     const DrawCallback& draw_renderables,
                                     const DrawCallback& draw_cursor) {
  glRenderer renderer;
  renderer.SetUp();
  renderer.ClearBuffer(std::make_shared<ScreenCanvas>(config.screen_size),
                       RGBAColour(0, 0, 0, 255));
  ShowGLErrors();

  glViewport(0, 0, config.display_size.width(), config.display_size.height());

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, static_cast<GLdouble>(config.display_size.width()),
          static_cast<GLdouble>(config.display_size.height()), 0.0, 0.0, 1.0);
  ShowGLErrors();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  ShowGLErrors();

  const auto aspect_ratio_w = static_cast<float>(config.display_size.width()) /
                              static_cast<float>(config.screen_size.width());
  const auto aspect_ratio_h = static_cast<float>(config.display_size.height()) /
                              static_cast<float>(config.screen_size.height());
  glTranslatef(config.screen_origin.x() * aspect_ratio_w,
               config.screen_origin.y() * aspect_ratio_h, 0.0f);

  if (draw_scene)
    draw_scene();
  if (draw_renderables)
    draw_renderables();

  if (config.manual_update_mode) {
    if (!screen_contents_texture_)
      screen_contents_texture_ =
          std::make_shared<glTexture>(config.display_size);

    glBindTexture(GL_TEXTURE_2D, screen_contents_texture_->GetID());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                        config.display_size.width(),
                        config.display_size.height());
    screen_contents_texture_valid_ = true;
  } else {
    screen_contents_texture_valid_ = false;
  }

  if (draw_cursor)
    draw_cursor();

  glFlush();
  SDL_GL_SwapBuffers();
  ShowGLErrors();
}

void SDLGraphicsBackend::RedrawLastFrame(const RenderFrameConfig& config,
                                         const DrawCallback& draw_cursor) {
  if (!config.manual_update_mode || !screen_contents_texture_valid_ ||
      !screen_contents_texture_)
    return;

  glRenderer renderer;
  renderer.Render(
      {screen_contents_texture_, Rect(Point(0, 0), config.display_size)},
      {std::make_shared<ScreenCanvas>(config.screen_size),
       Rect(Point(0, 0), config.screen_size)});

  if (draw_cursor)
    draw_cursor();

  SDL_GL_SwapBuffers();
  ShowGLErrors();
}

std::shared_ptr<Surface> SDLGraphicsBackend::RenderToSurface(
    const RenderFrameConfig& config,
    const DrawCallback& draw_scene) {
  auto canvas = std::make_shared<glCanvas>(
      config.screen_size, config.display_size, config.screen_origin);
  canvas->Use();

  const std::shared_ptr<glFrameBuffer> original_screen = SDLSurface::screen_;
  SDLSurface::screen_ = canvas->GetBuffer();
  if (draw_scene)
    draw_scene();
  SDLSurface::screen_ = original_screen;

  std::shared_ptr<glTexture> texture = canvas->GetBuffer()->GetTexture();
  const int width = config.screen_size.width();
  const int height = config.screen_size.height();

  std::vector<GLubyte> buf(width * height * 4);
  glGetTextureSubImage(texture->GetID(), 0, 0, 0, 0, width, height, 1, GL_RGBA,
                       GL_UNSIGNED_BYTE, buf.size(), buf.data());

  SDL_Surface* surface =
      SDL_CreateRGBSurface(width, height, 32, width * 4, 0xFF000000, 0x00FF0000,
                           0x0000FF00, 0x000000FF);
  if (!surface)
    throw std::runtime_error("SDL_CreateRGBSurface failed");

  for (int y = 0; y < height; ++y) {
    void* dst = static_cast<uint8_t*>(surface->pixels) + y * surface->pitch;
    const void* src = buf.data() + (height - y - 1) * width * 4;
    std::memcpy(dst, src, width * 4);
  }

  return std::make_shared<SDLSurface>(surface);
}
