// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "systems/base/text_factory.hpp"

#include "core/gameexe.hpp"
#include "log/domain_logger.hpp"
#include "machine/rlmachine.hpp"
#include "modules/jump.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/text_system.hpp"
#include "systems/base/text_waku_normal.hpp"
#include "systems/base/text_waku_type4.hpp"
#include "systems/base/text_window.hpp"
#include "systems/base/text_window_button.hpp"
#include "systems/event_system.hpp"
#include "utilities/string_utilities.hpp"

#include <format>

TextFactory::TextFactory(Gameexe& gexe) : gexe_(gexe) {}

std::unique_ptr<TextWaku> TextFactory::CreateWaku(System& system,
                                                  TextWindow& window,
                                                  int setno,
                                                  int no) {
  auto waku = gexe_("WAKU", setno, "TYPE");

  if (waku.Int().value_or(5) == 5)
    return CreateWakuNormal(system, window, setno, no);
  else
    return CreateWakuType4(system, window, setno, no);
}

// -----------------------------------------------------------------------
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

std::unique_ptr<TextWaku> TextFactory::CreateWakuNormal(System& system,
                                                        TextWindow& window,
                                                        int setno,
                                                        int no) {
  using namespace std::chrono_literals;
  auto result = std::make_unique<TextWakuNormal>();

  // Attach waku surface and action buttons defined in the
  // #WAKU.index1.index2.XXX_BOX properties. These actions represent
  // things such as moving the text box, clearing the text box, moving
  // forward or backwards in message history, and farcall()-ing a
  // custom handler (EXBTN_index_BOX).

  GraphicsSystem& gs = system.graphics();
  TextSystem& ts = system.text();
  std::shared_ptr<Clock> clock = system.event().GetClock();

  std::shared_ptr<Surface> main_surface, back_surface, btn_surface;
  auto waku = gexe_("WAKU", setno, no);
  if (auto name = waku("NAME").Str(); name.has_value() && !name.value().empty())
    main_surface = gs.GetSurfaceNamed(name.value());
  if (auto name = waku("BACK").Str(); name.has_value() && !name.value().empty())
    back_surface = gs.GetSurfaceNamed(name.value());
  if (auto name = waku("BTN").Str(); name.has_value() && !name.value().empty())
    btn_surface = gs.GetSurfaceNamed(name.value());

  result->SetWakuMain(main_surface);
  result->SetWakuBacking(back_surface);

  const Size text_surf_size = window.GetTextSurfaceSize();
  const Rect window_rect =
      window.GetWindowRect(result->GetSize(text_surf_size));
  std::string btn_name = "CLEAR_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<TextWindowButton>(
        clock, ts.window_clear_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_release_ = [&gs]() { gs.ToggleInterfaceHidden(); };
    btn->SetSurface(btn_surface, 8);
    result->AddButton(btn_name, std::move(btn));
  }

  btn_name = "MSGBKLEFT_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<TextWindowButton>(
        clock, ts.window_msgbkleft_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_pressed_ = [&ts] { ts.BackPage(); };
    btn->time_between_invocations_ = 250ms;
    btn->SetSurface(btn_surface, 24);
    result->AddButton(btn_name, std::move(btn));
  }

  btn_name = "MSGBKRIGHT_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<TextWindowButton>(
        clock, ts.window_msgbkright_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_pressed_ = [&ts] { ts.ForwardPage(); };
    btn->time_between_invocations_ = 250ms;
    btn->SetSurface(btn_surface, 32);
    result->AddButton(btn_name, std::move(btn));
  }

  for (int i = 0; i < 7; ++i) {
    auto wbcall = gexe_("WBCALL", i);
    btn_name = std::format("EXBTN_{:0>3}_BOX", i);
    if (waku(btn_name).Exists()) {
      int scenario = wbcall.IntAt(0).value_or(0);
      int entrypoint = wbcall.IntAt(1).value_or(0);
      auto btn = std::make_unique<TextWindowButton>(
          clock, ts.window_exbtn_use(),
          ParseButtonRect(window_rect, waku, btn_name));
      btn->on_release_ = [&system, scenario, entrypoint] {
        if (!system.machine_)
          return;
        system.text().set_system_visible(false);
        system.machine_->PushLongOperation(
            std::make_shared<RestoreTextSystemVisibility>());
        Farcall(*system.machine_, scenario, entrypoint);
      };
      btn->SetSurface(btn_surface, 40 + i * 8);
      result->AddButton(btn_name, std::move(btn));
    }
  }

  btn_name = "READJUMP_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<TextWindowButton>(
        clock, ts.window_read_jump_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_release_ = [&ts] { ts.SetSkipMode(!ts.skip_mode()); };
    btn->on_update_ = [&ts](TextWindowButton& btn) {
      const bool can_use = ts.kidoku_read(), on = ts.skip_mode();
      if (!can_use)
        btn.state_ = ButtonState::Disabled;
      else if (btn.state_ == ButtonState::Disabled)
        btn.state_ = ButtonState::Normal;
      if (on)
        btn.state_ = ButtonState::Activated;
      else if (btn.state_ == ButtonState::Activated)
        btn.state_ = ButtonState::Normal;
    };
    btn->on_update_(*btn);
    btn->SetSurface(btn_surface, 104);
    result->AddButton(btn_name, std::move(btn));
  }

  btn_name = "AUTOMODE_BOX";
  if (waku(btn_name).Exists()) {
    auto btn = std::make_unique<TextWindowButton>(
        clock, ts.window_automode_use(),
        ParseButtonRect(window_rect, waku, btn_name));
    btn->on_release_ = [&ts] { ts.SetAutoMode(!ts.auto_mode()); };
    btn->on_update_ = [&ts](TextWindowButton& btn) {
      bool automode = ts.auto_mode();
      if (automode)
        btn.state_ = ButtonState::Activated;
      else if (btn.state_ == ButtonState::Activated)
        btn.state_ = ButtonState::Normal;
    };
    btn->on_update_(*btn);
    btn->SetSurface(btn_surface, 112);
    result->AddButton(btn_name, std::move(btn));
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

  return result;
}

// -----------------------------------------------------------------------
std::unique_ptr<TextWaku> TextFactory::CreateWakuType4(System& system,
                                                       TextWindow& window,
                                                       int setno,
                                                       int no) {
  auto result = std::make_unique<TextWakuType4>();

  auto waku = gexe_("WAKU", setno, no);
  std::string waku_main_name = waku("NAME").Str().value_or("");

  result->SetMainWaku(waku_main_name.empty()
                          ? nullptr
                          : system.graphics().GetSurfaceNamed(waku_main_name));

  std::vector<int> waku_area =
      waku("AREA").IntVec().value_or(std::vector<int>{});
  int top = 0, bottom = 0, left = 0, right = 0;
  if (waku_area.size() >= 1)
    top = waku_area.at(0);
  if (waku_area.size() >= 2)
    bottom = waku_area.at(1);
  if (waku_area.size() >= 3)
    left = waku_area.at(2);
  if (waku_area.size() >= 4)
    right = waku_area.at(3);
  result->SetArea(top, bottom, left, right);

  return result;
}
