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

#include "core/rect.hpp"
#include "systems/base/text_window_button.hpp"
#include "systems/sdl_surface.hpp"

#include <algorithm>
#include <memory>
#include <string>

using namespace std::chrono_literals;

// -----------------------------------------------------------------------
// TextWakuNormal
// -----------------------------------------------------------------------
TextWakuNormal::TextWakuNormal(int setno, int no) : setno_(setno), no_(no) {}

void TextWakuNormal::AddButton(std::string btn_name,
                               int waku_offset,
                               std::unique_ptr<TextWindowButton> btn_impl) {
  if (!btn_impl)
    return;
  buttons_.emplace_back(std::move(btn_name), waku_offset, std::move(btn_impl));
}

void TextWakuNormal::Execute() {
  for (auto& btn : buttons_)
    btn.btn->Execute();
}

void TextWakuNormal::Render(Point box_location,
                            Size namebox_size,
                            RGBAColour colour,
                            bool is_filter) {
  if (backing_surface_) {
    Size backing_size = backing_surface_->GetSize();
    backing_surface_->RenderToScreenAsColorMask(
        Rect(Point(0, 0), backing_size), Rect(box_location, backing_size),
        colour, is_filter);
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

bool TextWakuNormal::HandleMouseClick(const Point& pos, bool pressed) {
  return std::any_of(buttons_.begin(), buttons_.end(), [&](auto& btn) {
    return btn.btn->HandleMouseClick(pos, pressed);
  });
}

void TextWakuNormal::SetWakuMain(std::shared_ptr<const Surface> surface) {
  main_surface_ = surface;
}

void TextWakuNormal::SetWakuBacking(std::shared_ptr<const Surface> surface) {
  if (!surface) {
    backing_surface_ = nullptr;
    return;
  }

  backing_surface_ = surface->Clone();
  backing_surface_->SetIsMask(true);
}

void TextWakuNormal::SetWakuButton(std::shared_ptr<const Surface> surface) {
  button_surface_ = surface;
}
