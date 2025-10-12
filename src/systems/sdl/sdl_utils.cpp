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

SDL_Color ToSDLColor(const RGBColour& in) {
  SDL_Color out;
  out.r = in.r();
  out.g = in.g();
  out.b = in.b();
  return out;
}
