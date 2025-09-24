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

#include "systems/sdl/sdl_text_window.hpp"

#include <SDL/SDL_ttf.h>

#include <string>
#include <vector>

#include "core/colour.hpp"
#include "core/gameexe.hpp"
#include "log/domain_logger.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/selection_element.hpp"
#include "systems/base/system_error.hpp"
#include "systems/base/text_window_button.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "systems/sdl/sdl_text_system.hpp"
#include "systems/sdl/sdl_utils.hpp"
#include "systems/sdl_surface.hpp"
#include "utf8.h"
#include "utilities/exception.hpp"
#include "utilities/string_utilities.hpp"

SDLTextWindow::SDLTextWindow(SDLSystem& system, int window_num)
    : TextWindow(system, window_num), sdl_system_(system) {
  ClearWin();
}

SDLTextWindow::~SDLTextWindow() {}

std::shared_ptr<Surface> SDLTextWindow::GetTextSurface() { return surface_; }

std::shared_ptr<Surface> SDLTextWindow::GetNameSurface() {
  return name_surface_;
}

void SDLTextWindow::ClearWin() {
  TextWindow::ClearWin();

  // Allocate the text window surface
  if (!surface_)
    surface_ = std::make_shared<Surface>(GetTextSurfaceSize());
  surface_->Fill(RGBAColour::Clear());

  name_surface_.reset();
}

void SDLTextWindow::RenderNameInBox(const std::string& utf8str) {
  RGBColour shadow = RGBAColour::Black().rgb();
  name_surface_ = sdl_system_.text().RenderText(utf8str, font_size_in_pixels(),
                                                0, 0, font_colour_, &shadow, 0);
}

void SDLTextWindow::AddSelectionItem(const std::string& utf8str,
                                     int selection_id) {
  std::shared_ptr<TTF_Font> font =
      sdl_system_.text().GetFontOfSize(font_size_in_pixels());

  // Render the incoming string for both selected and not-selected.
  SDL_Color colour;
  RGBColourToSDLColor(font_colour_, &colour);

  SDL_Surface* normal =
      TTF_RenderUTF8_Blended(font.get(), utf8str.c_str(), colour);

  // Copy and invert the surface for whatever.
  SDL_Surface* inverted = AlphaInvert(normal);

  // Figure out xpos and ypos
  Point position = GetTextSurfaceRect().origin() +
                   Size(text_insertion_point_x_, text_insertion_point_y_);
  text_insertion_point_y_ += (font_size_in_pixels_ + y_spacing_ + ruby_size_);

  std::unique_ptr<SelectionElement> element =
      std::make_unique<SelectionElement>(
          system(), std::make_shared<Surface>(normal),
          std::make_shared<Surface>(inverted), selectionCallback(),
          selection_id, position);
  selections_.emplace_back(std::move(element));
}

void SDLTextWindow::DisplayRubyText(const std::string& utf8str) {
  if (ruby_begin_point_ != -1) {
    std::shared_ptr<TTF_Font> font =
        sdl_system_.text().GetFontOfSize(ruby_text_size());
    int end_point = text_insertion_point_x_ - x_spacing_;

    if (ruby_begin_point_ > end_point) {
      ruby_begin_point_ = -1;
      throw rlvm::Exception("We don't handle ruby across line breaks yet!");
    }

    SDL_Color colour;
    RGBColourToSDLColor(font_colour_, &colour);
    SDL_Surface* tmp =
        TTF_RenderUTF8_Blended(font.get(), utf8str.c_str(), colour);

    // Render glyph to surface
    int w = tmp->w;
    int h = tmp->h;
    int height_location = text_insertion_point_y_ - ruby_text_size();
    int width_start =
        int(ruby_begin_point_ + ((end_point - ruby_begin_point_) * 0.5f) -
            (w * 0.5f));
    surface_->blitFROMSurface(
        tmp, Rect(Point(0, 0), Size(w, h)),
        Rect(Point(width_start, height_location), Size(w, h)), 255);
    SDL_FreeSurface(tmp);

    ruby_begin_point_ = -1;
  }

  last_token_was_name_ = false;
}

bool SDLTextWindow::DisplayCharacter(const std::string& current,
                                     const std::string& rest) {
  // If this text page is already full, save some time and reject
  // early.
  if (IsFull())
    return false;

  set_is_visible(true);

  if (current != "") {
    int cur_codepoint = Codepoint(current);
    bool indent_after_spacing = false;

    // But if the last character was a lenticular bracket, we need to indent
    // now. See doc/notes/NamesAndIndentation.txt for more details.
    if (last_token_was_name_) {
      if (name_mod_ == 0) {
        if (IsOpeningQuoteMark(cur_codepoint))
          indent_after_spacing = true;
      } else if (name_mod_ == 2) {
        if (IsOpeningQuoteMark(cur_codepoint)) {
          indent_after_spacing = true;
        }
      }
    }

    // If the width of this glyph plus the spacing will put us over the
    // edge of the window, then line increment.
    if (MustLineBreak(cur_codepoint, rest)) {
      HardBrake();

      if (IsFull())
        return false;
    }

    RGBColour shadow = RGBAColour::Black().rgb();
    sdl_system_.text().RenderGlyphOnto(
        current, font_size_in_pixels(), next_char_italic_, font_colour_,
        &shadow, text_insertion_point_x_, text_insertion_point_y_,
        GetTextSurface());
    next_char_italic_ = false;
    text_wrapping_point_x_ += GetWrappingWidthFor(cur_codepoint);

    if (cur_codepoint < 127) {
      // This is a basic ASCII character. In normal RealLive, western text
      // appears to be treated as half width monospace. If we're here, we are
      // either in a manually laid out western game (therefore we should try to
      // fit onto the monospace grid) or we're in rlbabel (in which case, our
      // insertion point will be manually set by the bytecode immediately after
      // this character).
      if (text_system_.FontIsMonospaced()) {
        // If our font is monospaced (ie msgothic.ttc), we want to follow the
        // game's layout instructions perfectly.
        text_insertion_point_x_ += GetWrappingWidthFor(cur_codepoint);
      } else {
        // If out font has different widths for 'i' and 'm', we aren't using
        // the recommended font so we'll try laying out the text so that
        // kerning looks better. This is the common case.
        text_insertion_point_x_ +=
            text_system_.GetCharWidth(font_size_in_pixels(), cur_codepoint);
      }
    } else {
      // Move the insertion point forward one character
      text_insertion_point_x_ += font_size_in_pixels_ + x_spacing_;
    }

    if (indent_after_spacing)
      SetIndentation();
  }

  // When we aren't rendering a piece of text with a ruby gloss, mark
  // the screen as dirty so that this character renders.
  if (ruby_begin_point_ == -1) {
    // system_.graphics().MarkScreenAsDirty(GUT_TEXTSYS);
  }

  last_token_was_name_ = false;

  return true;
}
