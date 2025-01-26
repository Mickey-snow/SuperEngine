// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
// Copyright (C) 2006 Elliot Glaysher
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

#include "systems/sdl/event_backend.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_events.h>

namespace {

inline MouseButton fromSDLButton(Uint8 sdlButton) {
  switch (sdlButton) {
    case SDL_BUTTON_LEFT:
      return MouseButton::LEFT;
    case SDL_BUTTON_RIGHT:
      return MouseButton::RIGHT;
    case SDL_BUTTON_MIDDLE:
      return MouseButton::MIDDLE;
    // case 4: return MouseButton::WHEELUP;
    // case 5: return MouseButton::WHEELDOWN;
    default:
      return MouseButton::NONE;
  }
}

inline KeyCode fromSDLKey(SDLKey sdlKey) {
  // Because the current KeyCode enum "lines up" with the old SDL1.2 key sym
  // values, a simple static_cast is  sufficient:
  return static_cast<KeyCode>(sdlKey);
}

Event translateSDLToEvent(const SDL_Event& sdlEvent) {
  switch (sdlEvent.type) {
    case SDL_QUIT:
      return Quit{};

    // window re-exposed after being covered/minimized
    case SDL_VIDEOEXPOSE:
      return VideoExpose{};

    case SDL_VIDEORESIZE:

      return VideoResize{Size{sdlEvent.resize.w, sdlEvent.resize.h}};

    // gain/lose focus, etc.
    case SDL_ACTIVEEVENT: {
      //   SDL_APPINPUTFOCUS  (0x01)
      //   SDL_APPACTIVE      (0x02)
      //   SDL_APPMOUSEFOCUS  (0x04)
      if (sdlEvent.active.state & SDL_APPINPUTFOCUS) {
        // Assume the mouse is inside the window. Actually checking the mouse
        // state doesn't work in the case where we mouse click on another window
        // that's partially covered by rlvm's window and then alt-tab back.
        return Active{true};
      } else if (sdlEvent.active.state & SDL_APPMOUSEFOCUS) {
        return Active{sdlEvent.active.gain == 1};
      }
      return std::monostate();
    }

    case SDL_KEYDOWN: {
      KeyDown kd;
      kd.code = fromSDLKey(sdlEvent.key.keysym.sym);
      return kd;
    }
    case SDL_KEYUP: {
      KeyUp ku;
      ku.code = fromSDLKey(sdlEvent.key.keysym.sym);
      return ku;
    }

    case SDL_MOUSEBUTTONDOWN: {
      MouseDown md;
      md.button = fromSDLButton(sdlEvent.button.button);
      return md;
    }
    case SDL_MOUSEBUTTONUP: {
      MouseUp mu;
      mu.button = fromSDLButton(sdlEvent.button.button);
      return mu;
    }

    case SDL_MOUSEMOTION: {
      MouseMotion mm;
      mm.pos = {sdlEvent.motion.x, sdlEvent.motion.y};
      return mm;
    }

    // Unhandled event type
    default:
      return std::monostate{};
  }
}

}  // namespace

SDLEventBackend::SDLEventBackend() = default;

std::shared_ptr<Event> SDLEventBackend::PollEvent() {
  SDL_Event event;
  if (SDL_PollEvent(&event))
    return std::make_shared<Event>(translateSDLToEvent(event));
  return std::make_shared<Event>(std::monostate());
}
