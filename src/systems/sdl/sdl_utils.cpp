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

#include "systems/sdl/sdl_utils.hpp"

#include <SDL/SDL.h>
#include "GL/glew.h"

#include <cassert>
#include <sstream>
#include <string>

#include "core/colour.hpp"
#include "core/rect.hpp"
#include "systems/base/system_error.hpp"

void reportSDLError(const std::string& sdl_name,
                    const std::string& function_name) {
  std::ostringstream ss;
  ss << "Error while calling SDL function '" << sdl_name << "' in "
     << function_name << ": " << SDL_GetError();
  throw SystemError(ss.str());
}

// -----------------------------------------------------------------------

/// TODO(erg): This is not endian safe in any way.
SDL_Surface* AlphaInvert(SDL_Surface* in_surface) {
  SDL_PixelFormat* format = in_surface->format;

  if (format->BitsPerPixel != 32)
    throw SystemError("AlphaInvert requires an alpha channel!");

  // Build a copy of the surface
  SDL_Surface* dst = SDL_AllocSurface(
      in_surface->flags, in_surface->w, in_surface->h, format->BitsPerPixel,
      format->Rmask, format->Gmask, format->Bmask, format->Amask);

  SDL_BlitSurface(in_surface, NULL, dst, NULL);

  // iterate over the copy and make the alpha value = 255 - alpha value.
  if (SDL_MUSTLOCK(dst))
    SDL_LockSurface(dst);
  {
    int num_pixels = dst->h * dst->pitch;
    char* p_data = (char*)dst->pixels;

    for (int i = 0; i < num_pixels; i += 4) {
      // Invert the pixel here.
      p_data[i] = 255 - p_data[i];
      p_data[i + 1] = 255 - p_data[i + 1];
      p_data[i + 2] = 255 - p_data[i + 2];
      p_data[i + 3] = 255 - p_data[i + 3];
    }
  }
  if (SDL_MUSTLOCK(dst))
    SDL_UnlockSurface(dst);

  return dst;
}

// -----------------------------------------------------------------------

void RectToSDLRect(const Rect& rect, SDL_Rect* out) {
  out->x = rect.x();
  out->y = rect.y();
  out->w = rect.width();
  out->h = rect.height();
}

// -----------------------------------------------------------------------

void RGBColourToSDLColor(const RGBColour& in, SDL_Color* out) {
  out->r = in.r();
  out->g = in.g();
  out->b = in.b();
}

// -----------------------------------------------------------------------

Uint32 MapRGBA(SDL_PixelFormat* fmt, const RGBAColour& in) {
  return SDL_MapRGBA(fmt, in.r(), in.g(), in.b(), in.a());
}
