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
#include "utilities/expected.hpp"
#include "utilities/string_utilities.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <memory>
#include <string>

using namespace std::chrono_literals;

// -----------------------------------------------------------------------
// TextWakuNormal
// -----------------------------------------------------------------------
TextWakuNormal::TextWakuNormal(System& system,
                               TextWindow& window,
                               int setno,
                               int no)
    : system_(system), window_(window), setno_(setno), no_(no) {
  LoadWindowWaku(system_.gameexe());
}

TextWakuNormal::~TextWakuNormal() = default;

void TextWakuNormal::AddButton(
    std::string btn_name,
    int waku_offset,
    std::unique_ptr<BasicTextWindowButton> btn_impl) {
  buttons_.emplace_back(std::move(btn_name), waku_offset, std::move(btn_impl));
}

void TextWakuNormal::Execute() {
  for (auto& btn : buttons_)
    btn.btn->Execute();
}

void TextWakuNormal::Render(Point box_location, Size namebox_size) {
  if (backing_surface_) {
    Size backing_size = backing_surface_->GetSize();
    backing_surface_->RenderToScreenAsColorMask(
        Rect(Point(0, 0), backing_size), Rect(box_location, backing_size),
        window_.colour(), window_.filter());
  }

  if (main_surface_) {
    Size main_size = main_surface_->GetSize();
    main_surface_->RenderToScreen(Rect(Point(0, 0), main_size),
                                  Rect(box_location, main_size), 255);
  }

  if (button_surface_)
    RenderButtons();
}

void TextWakuNormal::RenderButtons() {
  for (auto& btn : buttons_)
    btn.btn->Render(button_surface_, btn.waku_offset);
}

Size TextWakuNormal::GetSize(const Size& text_surface) const {
  if (main_surface_)
    return main_surface_->GetSize();
  else if (backing_surface_)
    return backing_surface_->GetSize();
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
  for (auto& btn : buttons_)
    btn.btn->SetMousePosition(pos);
}

bool TextWakuNormal::HandleMouseClick(RLMachine& machine,
                                      const Point& pos,
                                      bool pressed) {
  return std::any_of(buttons_.begin(), buttons_.end(), [&](auto& btn) {
    return btn.btn->HandleMouseClick(machine, pos, pressed);
  });
}

static Rect ParseButtonRect(Rect window_rect,
                            GameexeInterpretObject& it,
                            const std::string& key) {
  static DomainLogger logger("ParseButtonRect");

  auto loc_param = it(key).IntVec();

  if (!loc_param.has_value()) {
    logger(Severity::Error) << "Failed to load " << key << ": "
                            << loc_param.error().GetDebugString();
    return {};
  }

  if (loc_param->size() != 5) {
    logger(Severity::Error)
        << "Expected 5 integers, but got: {"
        << Join(",", view_to_string(loc_param.value())) << '}';
    return {};
  }

  int type = loc_param->at(0);
  Size size(loc_param->at(3), loc_param->at(4));

  switch (type) {
    case 0: {
      // Top and left
      Point origin =
          window_rect.origin() + Size(loc_param->at(1), loc_param->at(2));
      return Rect(origin, size);
    }
    case 1: {
      // Top and right
      Point origin = window_rect.origin() +
                     Size(-loc_param->at(1), loc_param->at(2)) +
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
                     Size(loc_param->at(1), -loc_param->at(2)) +
                     Size(0, window_rect.size().height() - size.height());
      return Rect(origin, size);
    }
    case 3: {
      // Bottom and right
      Point origin = window_rect.origin() +
                     Size(-loc_param->at(1), -loc_param->at(2)) +
                     Size(window_rect.size().width() - size.width(),
                          window_rect.size().height() - size.height());
      return Rect(origin, size);
    }
    default: {
      logger(Severity::Error) << "Unsupported coordinate system type=" << type;
      return {};
    }
  }
}

// Attach waku surface and action buttons defined in the
// #WAKU.index1.index2.XXX_BOX properties. These actions represent
// things such as moving the text box, clearing the text box, moving
// forward or backwards in message history, and farcall()-ing a
// custom handler (EXBTN_index_BOX).
void TextWakuNormal::LoadWindowWaku(Gameexe& gexe) {
  // TODO: Extract as a factory

  GameexeInterpretObject waku(gexe("WAKU", setno_, no_));

  SetWakuMain(waku("NAME").Str().value_or(""));
  SetWakuBacking(waku("BACK").Str().value_or(""));
  SetWakuButton(waku("BTN").Str().value_or(""));

  TextSystem& ts = system_.text();
  GraphicsSystem& gs = system_.graphics();
  std::shared_ptr<Clock> clock = system_.event().GetClock();

  const Rect window_rect = window_.GetWindowRect();
  std::string btn_name;
  buttons_.reserve(12);

  btn_name = "CLEAR_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<BasicTextWindowButton>(
        clock, ts.window_clear_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_release_ = [&gs]() { gs.ToggleInterfaceHidden(); };
    AddButton(btn_name, 8, std::move(btn));
  }

  btn_name = "MSGBKLEFT_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<RepeatActionWhileHoldingWindowButton>(
        clock, ts.window_msgbkleft_use(),
        ParseButtonRect(window_rect, waku, btn_name),
        [&ts]() { ts.BackPage(); }, 250ms);
    AddButton(btn_name, 24, std::move(btn));
  }

  btn_name = "MSGBKRIGHT_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<RepeatActionWhileHoldingWindowButton>(
        clock, ts.window_msgbkright_use(),
        ParseButtonRect(window_rect, waku, btn_name),
        [&ts]() { ts.ForwardPage(); }, 250ms);
    AddButton(btn_name, 32, std::move(btn));
  }

  for (int i = 0; i < 7; ++i) {
    GameexeInterpretObject wbcall(gexe("WBCALL", i));
    btn_name = std::format("EXBTN_{:0>3}_BOX", i);
    if (waku(btn_name).Exists()) {
      int scenario = wbcall.IntAt(0).value_or(0);
      int entrypoint = wbcall.IntAt(1).value_or(0);
      auto btn = std::make_unique<ExbtnWindowButton>(
          clock, ts.window_exbtn_use(),
          ParseButtonRect(window_rect, waku, btn_name), scenario, entrypoint);
      AddButton(btn_name, 40 + i * 8, std::move(btn));
    }
  }

  btn_name = "READJUMP_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<ActivationTextWindowButton>(
        clock, ts.window_read_jump_use(),
        ParseButtonRect(window_rect, waku, btn_name),
        [&ts](int skip_mode) { ts.SetSkipMode(skip_mode); });
    btn->SetEnabledNotification(NotificationType::SKIP_MODE_ENABLED_CHANGED);
    btn->SetChangeNotification(NotificationType::SKIP_MODE_STATE_CHANGED);

    // Set the initial enabled state. If true, we'll get a signal enabling it
    // immediately.
    btn->SetEnabled(ts.kidoku_read());

    AddButton(btn_name, 104, std::move(btn));
  }

  btn_name = "AUTOMODE_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<ActivationTextWindowButton>(
        clock, ts.window_automode_use(),
        ParseButtonRect(window_rect, waku, btn_name),
        [&ts](int auto_mode) { ts.SetAutoMode(auto_mode); });
    btn->SetChangeNotification(NotificationType::AUTO_MODE_STATE_CHANGED);

    AddButton(btn_name, 112, std::move(btn));
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

  std::erase_if(buttons_, [](auto& entry) { return entry.btn == nullptr; });
}

void TextWakuNormal::SetWakuMain(const std::string& name) {
  if (!name.empty())
    main_surface_ = system_.graphics().GetSurfaceNamed(name);
  else
    main_surface_.reset();
}

void TextWakuNormal::SetWakuBacking(const std::string& name) {
  if (!name.empty()) {
    backing_surface_ = system_.graphics().GetSurfaceNamed(name)->Clone();
    backing_surface_->SetIsMask(true);
  } else {
    backing_surface_.reset();
  }
}

void TextWakuNormal::SetWakuButton(const std::string& name) {
  if (!name.empty())
    button_surface_ = system_.graphics().GetSurfaceNamed(name);
  else
    button_surface_.reset();
}
