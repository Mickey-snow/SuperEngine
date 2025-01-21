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
#include "systems/base/event_listener.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_system.hpp"

#include <functional>

using std::bind;
using std::placeholders::_1;

SDLEventSystem::SDLEventSystem(SDLSystem& sys)
    : EventSystem(),
      shift_pressed_(false),
      ctrl_pressed_(false),
      mouse_inside_window_(true),
      mouse_pos_(),
      button1_state_(0),
      button2_state_(0),
      last_get_currsor_time_(0),
      last_mouse_move_time_(0),
      system_(sys) {}

void SDLEventSystem::ExecuteEventSystem(RLMachine& machine) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
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
  }
}

bool SDLEventSystem::CtrlPressed() const {
  return system_.force_fast_forward() || ctrl_pressed_;
}

Point SDLEventSystem::GetCursorPos() {
  PreventCursorPosSpinning();
  return mouse_pos_;
}

void SDLEventSystem::GetCursorPos(Point& position, int& button1, int& button2) {
  PreventCursorPosSpinning();
  position = mouse_pos_;
  button1 = button1_state_;
  button2 = button2_state_;
}

void SDLEventSystem::FlushMouseClicks() {
  button1_state_ = 0;
  button2_state_ = 0;
}

unsigned int SDLEventSystem::TimeOfLastMouseMove() {
  return last_mouse_move_time_;
}

bool SDLEventSystem::ShiftPressed() const { return shift_pressed_; }

void SDLEventSystem::PreventCursorPosSpinning() {
  unsigned int newTime = GetTicks();

  if ((system_.graphics().screen_update_mode() !=
       GraphicsSystem::SCREENUPDATEMODE_MANUAL) &&
      (newTime - last_get_currsor_time_) < 20) {
    // Prevent spinning on input. When we're not in manual mode, we don't get
    // convenient refresh() calls to insert pauses at. Instead, we need to sort
    // of intuit about what's going on and the easiest way to slow down is to
    // track when the bytecode keeps spamming us for the cursor.
    system_.set_force_wait(true);
  }

  last_get_currsor_time_ = newTime;
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

  KeyCode code = KeyCode(event.key.keysym.sym);
  DispatchEvent(bind(&EventListener::KeyStateChanged, _1, code, true));
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

  KeyCode code = KeyCode(event.key.keysym.sym);
  DispatchEvent(bind(&EventListener::KeyStateChanged, _1, code, false));
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

    // Handle this somehow.
    BroadcastEvent(bind(&EventListener::MouseMotion, _1, mouse_pos_));
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

    MouseButton button = MouseBtn::NONE;
    switch (event.button.button) {
      case SDL_BUTTON_LEFT:
        button = MouseBtn::LEFT;
        break;
      case SDL_BUTTON_RIGHT:
        button = MouseBtn::RIGHT;
        break;
      case SDL_BUTTON_MIDDLE:
        button = MouseBtn::MIDDLE;
        break;
      case SDL_BUTTON_WHEELUP:
        button = MouseBtn::WHEELUP;
        break;
      case SDL_BUTTON_WHEELDOWN:
        button = MouseBtn::WHEELDOWN;
        break;
      default:
        break;
    }

    DispatchEvent(
        bind(&EventListener::MouseButtonStateChanged, _1, button, pressed));
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
