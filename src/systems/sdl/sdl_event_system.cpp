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

#include "systems/sdl/sdl_event_system.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_events.h>

#include "machine/rlmachine.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_system.hpp"

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
      bool inputFocus = (sdlEvent.active.state & SDL_APPINPUTFOCUS) != 0;
      bool mouseFocus = (sdlEvent.active.state & SDL_APPMOUSEFOCUS) != 0;
      return Active{inputFocus, mouseFocus};
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

SDLEventSystem::SDLEventSystem() = default;

void SDLEventSystem::ExecuteEventSystem(RLMachine& machine) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // TODO: consider move the following logic into individual event listeners
    switch (event.type) {
      case SDL_KEYDOWN: {
        HandleKeyDown(machine, event);
        break;
      }
      case SDL_KEYUP: {
        HandleKeyUp(machine, event);
        break;
      }
      case SDL_MOUSEMOTION: {
        HandleMouseMotion(machine, event);
        break;
      }
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP: {
        HandleMouseButtonEvent(machine, event);
        break;
      }
      case SDL_QUIT:
        machine.Halt();
        break;
      case SDL_ACTIVEEVENT:
        HandleActiveEvent(machine, event);
        break;
      case SDL_VIDEOEXPOSE:
        machine.GetSystem().graphics().ForceRefresh();
        break;
      case SDL_VIDEORESIZE:
        Size new_size = Size(event.resize.w, event.resize.h);
        dynamic_cast<SDLGraphicsSystem&>(machine.GetSystem().graphics())
            .Resize(new_size);
        break;
    }

    auto event_ptr = std::make_shared<Event>(translateSDLToEvent(event));
    EventSystem::DispatchEvent(event_ptr);
  }
}

void SDLEventSystem::HandleKeyDown(RLMachine& machine, SDL_Event& event) {
  switch (event.key.keysym.sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT: {
      shift_pressed_ = true;
      break;
    }
    case SDLK_LCTRL:
    case SDLK_RCTRL: {
      ctrl_pressed_ = true;
      break;
    }
    case SDLK_RETURN:
    case SDLK_f: {
      if ((event.key.keysym.mod & KMOD_ALT) ||
          (event.key.keysym.mod & KMOD_META)) {
        machine.GetSystem().graphics().ToggleFullscreen();

        // Stop processing because we don't want to Dispatch this event, which
        // might advance the text.
        return;
      }

      break;
    }
    default:
      break;
  }
}

void SDLEventSystem::HandleKeyUp(RLMachine& machine, SDL_Event& event) {
  switch (event.key.keysym.sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT: {
      shift_pressed_ = false;
      break;
    }
    case SDLK_LCTRL:
    case SDLK_RCTRL: {
      ctrl_pressed_ = false;
      break;
    }
    case SDLK_F1: {
      machine.GetSystem().ShowSystemInfo(machine);
      break;
    }
    default:
      break;
  }
}

void SDLEventSystem::HandleMouseMotion(RLMachine& machine, SDL_Event& event) {
  if (mouse_inside_window_) {
    last_mouse_move_time_ = GetTicks();

    const auto& graphics_sys =
        dynamic_cast<SDLGraphicsSystem&>(machine.GetSystem().graphics());
    const auto aspect_ratio_w = 1.0f * graphics_sys.GetDisplaySize().width() /
                                graphics_sys.screen_size().width();
    const auto aspect_ratio_h = 1.0f * graphics_sys.GetDisplaySize().height() /
                                graphics_sys.screen_size().height();
    mouse_pos_ =
        Point(event.motion.x / aspect_ratio_w, event.motion.y / aspect_ratio_h);
  }
}

void SDLEventSystem::HandleMouseButtonEvent(RLMachine& machine,
                                            SDL_Event& event) {
  if (mouse_inside_window_) {
    bool pressed = event.type == SDL_MOUSEBUTTONDOWN;
    int press_code = pressed ? 1 : 2;

    if (event.button.button == SDL_BUTTON_LEFT)
      button1_state_ = press_code;
    else if (event.button.button == SDL_BUTTON_RIGHT)
      button2_state_ = press_code;
  }
}

void SDLEventSystem::HandleActiveEvent(RLMachine& machine, SDL_Event& event) {
  if (event.active.state & SDL_APPINPUTFOCUS) {
    // Assume the mouse is inside the window. Actually checking the mouse
    // state doesn't work in the case where we mouse click on another window
    // that's partially covered by rlvm's window and then alt-tab back.
    mouse_inside_window_ = true;

  } else if (event.active.state & SDL_APPMOUSEFOCUS) {
    mouse_inside_window_ = event.active.gain == 1;
  }
}
