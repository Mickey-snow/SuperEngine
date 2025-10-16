// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#include "systems/base/text_waku_normal.hpp"

#include "core/gameexe.hpp"
#include "core/rect.hpp"
#include "log/domain_logger.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/text_system.hpp"
#include "systems/base/text_window.hpp"
#include "systems/base/text_window_button.hpp"
#include "systems/event_system.hpp"
#include "systems/sdl_surface.hpp"

#include <format>
#include <functional>
#include <memory>
#include <string>

using namespace std::chrono_literals;

namespace {

// Definitions for the location and Gameexe.ini keys describing various text
// window buttons.
//
// Previously was using a map keyed on strings. In rendering code. With keys
// that had similar prefixes. WTF was I smoking...
static struct ButtonInfo {
  int index;
  const char* button_name;
  int waku_offset;
} BUTTON_INFO[] = {{0, "CLEAR_BOX", 8},
                   {1, "MSGBKLEFT_BOX", 24},
                   {2, "MSGBKRIGHT_BOX", 32},
                   {3, "EXBTN_000_BOX", 40},
                   {4, "EXBTN_001_BOX", 48},
                   {5, "EXBTN_002_BOX", 56},
                   {6, "EXBTN_003_BOX", 64},
                   {7, "EXBTN_004_BOX", 72},
                   {8, "EXBTN_005_BOX", 80},
                   {9, "EXBTN_006_BOX", 88},
                   {10, "READJUMP_BOX", 104},
                   {11, "AUTOMODE_BOX", 112},
                   {-1, NULL, -1}};

}  // namespace

// -----------------------------------------------------------------------
// TextWakuNormal
// -----------------------------------------------------------------------
TextWakuNormal::TextWakuNormal(System& system,
                               TextWindow& window,
                               int setno,
                               int no)
    : system_(system), window_(window), setno_(setno), no_(no) {
  LoadWindowWaku();
}

TextWakuNormal::~TextWakuNormal() {}

void TextWakuNormal::Execute() {
  for (int i = 0; BUTTON_INFO[i].index != -1; ++i) {
    if (button_map_[i]) {
      button_map_[i]->Execute();
    }
  }
}

void TextWakuNormal::Render(Point box_location, Size namebox_size) {
  if (waku_backing_) {
    Size backing_size = waku_backing_->GetSize();
    waku_backing_->RenderToScreenAsColorMask(
        Rect(Point(0, 0), backing_size), Rect(box_location, backing_size),
        window_.colour(), window_.filter());
  }

  if (waku_main_) {
    Size main_size = waku_main_->GetSize();
    waku_main_->RenderToScreen(Rect(Point(0, 0), main_size),
                               Rect(box_location, main_size), 255);
  }

  if (waku_button_)
    RenderButtons();
}

void TextWakuNormal::RenderButtons() {
  for (int i = 0; BUTTON_INFO[i].index != -1; ++i) {
    if (button_map_[i]) {
      button_map_[i]->Render(waku_button_, BUTTON_INFO[i].waku_offset);
    }
  }
}

Size TextWakuNormal::GetSize(const Size& text_surface) const {
  if (waku_main_)
    return waku_main_->GetSize();
  else if (waku_backing_)
    return waku_backing_->GetSize();
  else
    return text_surface;
}

Point TextWakuNormal::InsertionPoint(const Rect& waku_rect,
                                     const Size& padding,
                                     const Size& surface_size,
                                     bool center) const {
  // In normal type 5 wakus, we just offset from the top left corner by padding
  // amounts.
  Point insertion_point = waku_rect.origin() + padding;
  if (center) {
    int half_width = (waku_rect.width() - 2 * padding.width()) / 2;
    int half_text_width = surface_size.width() / 2;
    insertion_point += Point(half_width - half_text_width, 0);
  }

  return insertion_point;
}

void TextWakuNormal::SetMousePosition(const Point& pos) {
  for (int i = 0; BUTTON_INFO[i].index != -1; ++i) {
    if (button_map_[i]) {
      button_map_[i]->SetMousePosition(pos);
    }
  }
}

bool TextWakuNormal::HandleMouseClick(RLMachine& machine,
                                      const Point& pos,
                                      bool pressed) {
  for (int i = 0; BUTTON_INFO[i].index != -1; ++i) {
    if (button_map_[i]) {
      if (button_map_[i]->HandleMouseClick(machine, pos, pressed))
        return true;
    }
  }

  return false;
}

static std::optional<Rect> GetButtonRect(Rect window_rect,
                                         std::vector<int> loc_param) {
  if (loc_param.size() != 5)
    return std::nullopt;
  int type = loc_param.at(0);
  Size size(loc_param.at(3), loc_param.at(4));
  switch (type) {
    case 0: {
      // Top and left
      Point origin =
          window_rect.origin() + Size(loc_param.at(1), loc_param.at(2));
      return Rect(origin, size);
    }
    case 1: {
      // Top and right
      Point origin = window_rect.origin() +
                     Size(-loc_param.at(1), loc_param.at(2)) +
                     Size(window_rect.size().width() - size.width(), 0);
      return Rect(origin, size);
    }
    // TODO(erg): While 0 is used in pretty much everything I've tried, and 1
    // is used in the Maiden Halo demo, I'm not certain about these other two
    // calculations. Both KareKare games screw up here, but it may be because
    // of the win_rect. Needs further investigation.
    case 2: {
      // Bottom and left
      Point origin = window_rect.origin() +
                     Size(loc_param.at(1), -loc_param.at(2)) +
                     Size(0, window_rect.size().height() - size.height());
      return Rect(origin, size);
    }
    case 3: {
      // Bottom and right
      Point origin = window_rect.origin() +
                     Size(-loc_param.at(1), -loc_param.at(2)) +
                     Size(window_rect.size().width() - size.width(),
                          window_rect.size().height() - size.height());
      return Rect(origin, size);
    }
    default: {
      static DomainLogger logger("GetButtonRect");
      logger(Severity::Error) << "Unsupported coordinate system type=" << type;
    }
  }
  return std::nullopt;
}

void TextWakuNormal::LoadWindowWaku() {
  GameexeInterpretObject waku(system_.gameexe()("WAKU", setno_, no_));

  SetWakuMain(waku("NAME").Str().value_or(""));
  SetWakuBacking(waku("BACK").Str().value_or(""));
  SetWakuButton(waku("BTN").Str().value_or(""));

  TextSystem& ts = system_.text();
  GraphicsSystem& gs = system_.graphics();

  std::shared_ptr<Clock> clock = system_.event().GetClock();
  if (waku("CLEAR_BOX").Exists()) {
    std::optional<Rect> btn_rect =
        GetButtonRect(window_.GetWindowRect(),
                      waku("CLEAR_BOX").IntVec().value_or(std::vector<int>{}));
    button_map_[0] = std::make_unique<ActionTextWindowButton>(
        clock, ts.window_clear_use(), btn_rect.value_or(Rect()),
        [&gs]() { gs.ToggleInterfaceHidden(); });
  }
  if (waku("MSGBKLEFT_BOX").Exists()) {
    std::optional<Rect> btn_rect = GetButtonRect(
        window_.GetWindowRect(),
        waku("MSGBKLEFT_BOX").IntVec().value_or(std::vector<int>{}));
    button_map_[1] = std::make_unique<RepeatActionWhileHoldingWindowButton>(
        clock, ts.window_msgbkleft_use(), btn_rect.value_or(Rect()),
        [&ts]() { ts.BackPage(); }, 250ms);
  }
  if (waku("MSGBKRIGHT_BOX").Exists()) {
    std::optional<Rect> btn_rect = GetButtonRect(
        window_.GetWindowRect(),
        waku("MSGBKRIGHT_BOX").IntVec().value_or(std::vector<int>{}));
    button_map_[2] = std::make_unique<RepeatActionWhileHoldingWindowButton>(
        clock, ts.window_msgbkright_use(), btn_rect.value_or(Rect()),
        [&ts]() { ts.ForwardPage(); }, 250ms);
  }

  for (int i = 0; i < 7; ++i) {
    GameexeInterpretObject wbcall(system_.gameexe()("WBCALL", i));
    const std::string key = std::format("EXBTN_{:0>3}_BOX", i);
    if (waku(key).Exists()) {
      std::optional<Rect> btn_rect =
          GetButtonRect(window_.GetWindowRect(),
                        waku(key).IntVec().value_or(std::vector<int>{}));
      int scenario = wbcall.IntAt(0).value_or(0);
      int entrypoint = wbcall.IntAt(1).value_or(0);
      button_map_[3 + i] = std::make_unique<ExbtnWindowButton>(
          clock, ts.window_exbtn_use(), btn_rect.value_or(Rect()), scenario,
          entrypoint);
    }
  }

  if (waku("READJUMP_BOX").Exists()) {
    std::optional<Rect> btn_rect = GetButtonRect(
        window_.GetWindowRect(),
        waku("READJUMP_BOX").IntVec().value_or(std::vector<int>{}));
    auto readjump_box = std::make_unique<ActivationTextWindowButton>(
        clock, ts.window_read_jump_use(), btn_rect.value_or(Rect()),
        [&ts](int skip_mode) { ts.SetSkipMode(skip_mode); });
    readjump_box->SetEnabledNotification(
        NotificationType::SKIP_MODE_ENABLED_CHANGED);
    readjump_box->SetChangeNotification(
        NotificationType::SKIP_MODE_STATE_CHANGED);

    // Set the initial enabled state. If true, we'll get a signal enabling it
    // immediately.
    readjump_box->SetEnabled(ts.kidoku_read());

    button_map_[10] = std::move(readjump_box);
  }

  if (waku("AUTOMODE_BOX").Exists()) {
    std::optional<Rect> btn_rect = GetButtonRect(
        window_.GetWindowRect(),
        waku("AUTOMODE_BOX").IntVec().value_or(std::vector<int>{}));
    auto automode_button = std::make_unique<ActivationTextWindowButton>(
        clock, ts.window_automode_use(), btn_rect.value_or(Rect()),
        [&ts](int auto_mode) { ts.SetAutoMode(auto_mode); });
    automode_button->SetChangeNotification(
        NotificationType::AUTO_MODE_STATE_CHANGED);

    button_map_[11] = std::move(automode_button);
  }

  /*
   * TODO: I didn't translate these to the new way of doing things. I don't
   * seem to be rendering them. Must deal with this later.
   *
  string key = "MOVE_BOX";
  button_map_.insert(
    key, new TextWindowButton(ts.window_move_use(), waku("MOVE_BOX")));

  key = string("MSGBK_BOX");
  button_map_.insert(
    key, new TextWindowButton(ts.window_msgbk_use(), waku("MSGBK_BOX")));
  */
}

void TextWakuNormal::SetWakuMain(const std::string& name) {
  if (name != "")
    waku_main_ = system_.graphics().GetSurfaceNamed(name);
  else
    waku_main_.reset();
}

void TextWakuNormal::SetWakuBacking(const std::string& name) {
  if (name != "") {
    waku_backing_ = system_.graphics().GetSurfaceNamed(name)->Clone();
    waku_backing_->SetIsMask(true);
  } else {
    waku_backing_.reset();
  }
}

void TextWakuNormal::SetWakuButton(const std::string& name) {
  if (name != "")
    waku_button_ = system_.graphics().GetSurfaceNamed(name);
  else
    waku_button_.reset();
}
