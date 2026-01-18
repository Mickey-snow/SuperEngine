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

#include "systems/base/text_window.hpp"

#include "core/gameexe.hpp"
#include "libreallive/alldefs.hpp"
#include "machine/rlmachine.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/selection_element.hpp"
#include "systems/base/sound_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/text_factory.hpp"
#include "systems/base/text_system.hpp"
#include "systems/base/text_waku.hpp"
#include "systems/itext_system.hpp"
#include "systems/sdl_surface.hpp"
#include "utilities/assertx.hpp"
#include "utilities/graphics.hpp"
#include "utilities/string_utilities.hpp"

#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct TextWindow::FaceSlot {
  explicit FaceSlot(const std::vector<int> vec)
      : x(vec.at(0)),
        y(vec.at(1)),
        is_behind(vec.at(2)),
        hide_other_windows(vec.at(3)),
        unknown(vec.at(4)) {}

  int x, y;

  // 0 if layered in front or window background. 1 if behind.
  int is_behind;

  // Speculation: This makes ALMA work correctly and doesn't appear to harm
  // P_BRIDE.
  int hide_other_windows;

  // Unknown.
  int unknown;

  // The current face loaded. NULL whenever no face is loaded.
  std::shared_ptr<const Surface> face_surface;
};

// -----------------------------------------------------------------------
// TextWindow
// -----------------------------------------------------------------------

TextWindow::TextWindow(System& system, int window_num, ITextSystem* text_impl)
    : text_impl_(text_impl),
      window_num_(window_num),
      layout_(0, 0, 0),
      last_token_was_name_(false),
      use_indentation_(0),
      colour_(),
      is_filter_(0),
      is_visible_(false),
      state_(State::Normal),
      next_char_italic_(false),
      system_(system),
      text_system_(system.text()) {
  ASSERTX_NE(text_impl, nullptr);

  Gameexe& gexe = system.gameexe();

  // POINT
  Size size = GetScreenSize(gexe);
  screen_width_ = size.width();
  screen_height_ = size.height();

  // Base form for everything to follow.
  GameexeInterpretObject window(gexe("WINDOW", window_num));

  // Handle: #WINDOW.index.ATTR_MOD, #WINDOW_ATTR, #WINDOW.index.ATTR
  window_attr_mod_ = window("ATTR_MOD").Int().value_or(0);
  if (window_attr_mod_ == 0)
    SetRGBAF(system.text().window_attr());
  else
    SetRGBAF(window("ATTR").ToIntVec());

  // parse layout
  {
    default_font_size_ = window("MOJI_SIZE").Int().value_or(25);
    set_font_size_in_pixels(default_font_size_);
    std::vector<int> moji_cnt = window("MOJI_CNT").ToIntVec();
    const int x_window_size_in_chars = moji_cnt.at(0);
    const int y_window_size_in_chars = moji_cnt.at(1);
    std::vector<int> moji_rep = window("MOJI_REP").ToIntVec();
    const int x_spacing = moji_rep.at(0);
    const int y_spacing = moji_rep.at(1);
    const int ruby_size = window("LUBY_SIZE").Int().value_or(0);

    const int layout_height =
        y_window_size_in_chars * (default_font_size_ + y_spacing + ruby_size);
    const int layout_width =
        x_window_size_in_chars * (default_font_size_ + x_spacing);
    const int layout_extended =
        layout_width +
        default_font_size_;  // There is one extra character in each line to
                             // accommodate squeezed punctuation.

    layout_ = TextLayout(layout_height, layout_width, layout_extended);
    layout_.font_size = default_font_size_;
    layout_.ruby_font_size = ruby_size;
    layout_.x_spacing = x_spacing;
    layout_.y_spacing = y_spacing;
  }

  SetTextboxPadding(window("MOJI_POS").ToIntVec());

  SetWindowPosition(window("POS").ToIntVec());

  SetDefaultTextColor(gexe("COLOR_TABLE", 0).ToIntVec());

  // INDENT_USE appears to default to on. See the first scene in the
  // game with Nagisa, paying attention to indentation; then check the
  // Gameexe.ini.
  set_use_indentation(window("INDENT_USE").Int().value_or(1));

  SetKeycursorMod(window("KEYCUR_MOD").ToIntVec());
  set_action_on_pause(window("R_COMMAND_MOD").Int().value_or(0));

  // Main textbox waku
  TextFactory waku_factory(gexe);
  waku_set_ = window("WAKU_SETNO").Int().value_or(0);
  textbox_waku_ = waku_factory.CreateWaku(system_, *this, waku_set_, 0);

  // Name textbox if that setting has been enabled.
  SetNameMod(window("NAME_MOD").Int().value_or(0));
  if (auto no = window("NAME_WAKU_SETNO").Int();
      name_mod_ == NameMode::SeparateWindow && no) {
    name_waku_set_ = *no;
    namebox_waku_ = waku_factory.CreateWaku(system_, *this, name_waku_set_, 0);
    SetNameSpacingBetweenCharacters(window("NAME_MOJI_REP").Int().value_or(0));
    SetNameboxPadding(window("NAME_MOJI_POS").ToIntVec());
    // Ignoring NAME_WAKU_MIN for now
    SetNameboxPosition(window("NAME_POS").ToIntVec());
    name_waku_dir_set_ = window("NAME_WAKU_DIR").Int().value_or(0);
    namebox_centering_ = window("NAME_CENTERING").Int().value_or(0);
    minimum_namebox_size_ = window("NAME_MOJI_MIN").Int().value_or(4);
    name_size_ = window("NAME_MOJI_SIZE").ToInt();
  }

  // Load #FACE information.
  for (auto it : gexe.Filter(window.key() + ".FACE")) {
    // Retrieve the face slot number
    std::vector<std::string> GetKeyParts = it.GetKeyParts();

    try {
      int slot = std::stoi(GetKeyParts.at(3));
      if (slot < kNumFaceSlots) {
        face_slot_[slot] = std::make_unique<FaceSlot>(it.ToIntVec());
      }
    } catch (...) {
      // Parsing failure. Ignore this key.
    }
  }

  ClearWin();
}

TextWindow::~TextWindow() = default;

void TextWindow::Execute() {
  if (IsVisible() && !system_.graphics().is_interface_hidden()) {
    textbox_waku_->Execute();
  }
}

void TextWindow::SetTextboxPadding(const std::vector<int>& pos_data) {
  upper_box_padding_ = pos_data.at(0);
  lower_box_padding_ = pos_data.at(1);
  left_box_padding_ = pos_data.at(2);
  right_box_padding_ = pos_data.at(3);
}

void TextWindow::SetName(const std::string& utf8name,
                         const std::string& next_char) {
  if (name_mod_ == NameMode::Inline) {
    std::string interpreted_name = text_system_.InterpretName(utf8name);

    // Display the name in one pass
    PrintTextToFunction(
        [this](const std::string& current, const std::string& rest) {
          return this->DisplayCharacter(current, rest);
        },
        interpreted_name, next_char);
    SetIndentation();
  }

  SetNameWithoutDisplay(utf8name);
}

void TextWindow::SetNameWithoutDisplay(const std::string& utf8name) {
  if (name_mod_ == NameMode::SeparateWindow) {
    std::string interpreted_name = text_system_.InterpretName(utf8name);

    namebox_characters_ = 0;
    try {
      namebox_characters_ =
          utf8::distance(interpreted_name.begin(), interpreted_name.end());
    } catch (...) {
      // If utf8name isn't a real UTF-8 string, possibly overestimate:
      namebox_characters_ = interpreted_name.size();
    }

    namebox_characters_ = std::max(namebox_characters_, minimum_namebox_size_);

    RenderNameInBox(interpreted_name);
  }

  last_token_was_name_ = true;
}

void TextWindow::RenderNameInBox(const std::string& utf8str) {
  RGBColour shadow = RGBAColour::Black().rgb();
  name_surface_ = system_.text().RenderText(utf8str, font_size_in_pixels(), 0,
                                            0, font_colour_, &shadow, 0);
}

void TextWindow::SetDefaultTextColor(const std::vector<int>& colour) {
  SetDefaultTextColor(RGBColour(colour.at(0), colour.at(1), colour.at(2)));
}

void TextWindow::SetFontColor(const std::vector<int>& colour) {
  SetFontColor(RGBColour(colour.at(0), colour.at(1), colour.at(2)));
}

void TextWindow::SetWindowPosition(const std::vector<int>& pos_data) {
  origin_ = pos_data.at(0);
  x_distance_from_origin_ = pos_data.at(1);
  y_distance_from_origin_ = pos_data.at(2);
}

Size TextWindow::GetTextWindowSize() const { return layout_.GetNormalSize(); }

Size TextWindow::GetTextSurfaceSize() const {
  return layout_.GetExtendedSize();
}

Rect TextWindow::GetWindowRect() const {
  ASSERTX_NE(textbox_waku_, nullptr);
  Size waku_size = textbox_waku_->GetSize(GetTextSurfaceSize());
  return GetWindowRect(waku_size);
}

Rect TextWindow::GetWindowRect(Size waku_size) const {
  // This absolutely needs to know the size of the on main backing waku if we
  // want to draw things correctly! If we are going to offset this text box
  // from the top or the bottom, we MUST know what the size of the image
  // graphic is if we want accurate calculations, because some image graphics
  // are significantly larger than GetTextWindowSize() + the paddings.
  //
  // RealLive is definitely correcting programmer errors which places textboxes
  // offscreen. For example, take P_BRAVE (please!): #WINDOW.002.POS=2:78,6,
  // and a waku that refers to PM_WIN.g00 for it's image. That image is 800x300
  // pixels. The image is still centered perfectly, even though it's supposed
  // to be shifted 78 pixels right since the origin is the bottom
  // left. Expanding this number didn't change the position offscreen.

  int x, y;
  switch (origin_) {
    case 0:
    case 2:
      x = x_distance_from_origin_;
      break;
    case 1:
    case 3:
      x = screen_width_ - waku_size.width() - x_distance_from_origin_;
      break;
    [[unlikely]] default:
      throw std::logic_error("Invalid origin = " + std::to_string(origin_));
  }

  switch (origin_) {
    case 0:  // Top and left
    case 1:  // Top and right
      y = y_distance_from_origin_;
      break;
    case 2:  // Bottom and left
    case 3:  // Bottom and right
      y = screen_height_ - waku_size.height() - y_distance_from_origin_;
      break;
    [[unlikely]] default:
      throw std::logic_error("Invalid origin = " + std::to_string(origin_));
  }

  // Now that we have the coordinate that the programmer wanted to position the
  // box at, possibly move the box so it fits on screen.
  if ((x + waku_size.width()) > screen_width_)
    x -= (x + waku_size.width()) - screen_width_;
  if (x < 0)
    x = 0;

  if ((y + waku_size.height()) > screen_height_)
    y -= (y + waku_size.height()) - screen_height_;
  if (y < 0)
    y = 0;

  return Rect(x, y, waku_size);
}

Rect TextWindow::GetTextSurfaceRect() const {
  Rect window = GetWindowRect();

  Point textOrigin =
      window.origin() + Size(left_box_padding_, upper_box_padding_);

  Size rectSize = GetTextSurfaceSize();
  rectSize += Size(right_box_padding_, lower_box_padding_);

  return Rect(textOrigin, rectSize);
}

Rect TextWindow::GetNameboxWakuRect() const {
  // Like the main GetWindowRect(), we need to ask the waku what size it wants
  // to be.
  Size boxSize = namebox_waku_->GetSize(GetNameboxTextArea());

  // The waku is offset from the top left corner of the text window.
  Rect r = GetWindowRect();
  return Rect(Point(r.x() + namebox_x_offset_,
                    r.y() + namebox_y_offset_ - boxSize.height()),
              boxSize);
}

Size TextWindow::GetNameboxTextArea() const {
  // TODO(erg): This seems excessively wide.
  return Size(
      2 * horizontal_namebox_padding_ + namebox_characters_ * name_size_,
      vertical_namebox_padding_ + name_size_);
}

void TextWindow::SetNameMod(const int in) {
  ASSERTX_LE(0, in);
  ASSERTX_LE(in, 2);
  name_mod_ = static_cast<NameMode>(in);
}

void TextWindow::SetNameSpacingBetweenCharacters(int pos_data) {
  name_x_spacing_ = pos_data;
}

void TextWindow::SetNameboxPadding(const std::vector<int>& pos_data) {
  if (pos_data.size() >= 1)
    horizontal_namebox_padding_ = pos_data.at(0);
  if (pos_data.size() >= 2)
    vertical_namebox_padding_ = pos_data.at(1);
}

void TextWindow::SetNameboxPosition(const std::vector<int>& pos_data) {
  namebox_x_offset_ = pos_data.at(0);
  namebox_y_offset_ = pos_data.at(1);
}

void TextWindow::SetKeycursorMod(const std::vector<int>& keycur) {
  keycursor_type_ = keycur.at(0);
  keycursor_pos_ = Point(keycur.at(1), keycur.at(2));
}

Point TextWindow::KeycursorPosition(const Size& cursor_size) const {
  switch (keycursor_type_) {
    case 0:
      return GetTextSurfaceRect().lower_right() - cursor_size;
    case 1:
      return GetTextSurfaceRect().origin() + layout_.GetInsertionPoint();
    case 2:
      return GetTextSurfaceRect().origin() + keycursor_pos_;
    [[unlikely]] default:
      throw std::logic_error("Invalid keycursor type = " +
                             std::to_string(keycursor_type_));
  }
}

void TextWindow::FaceOpen(const std::string& filename, int index) {
  if (face_slot_[index]) {
    face_slot_[index]->face_surface =
        system_.graphics().GetSurfaceNamed(filename);

    if (face_slot_[index]->hide_other_windows) {
      system_.text().HideAllTextWindowsExcept(window_number());
    }
  }
}

void TextWindow::FaceClose(int index) {
  if (face_slot_[index]) {
    face_slot_[index]->face_surface.reset();

    if (face_slot_[index]->hide_other_windows) {
      system_.text().HideAllTextWindowsExcept(window_number());
    }
  }
}

void TextWindow::NextCharIsItalic() { next_char_italic_ = true; }

// TODO(erg): Make this pass the #WINDOW_ATTR colour off wile rendering the
// waku_backing.
void TextWindow::Render() {
  if (text_surface_ && IsVisible()) {
    Size surface_size = text_surface_->GetSize();

    // POINT
    Point box = GetWindowRect().origin();

    Point textOrigin = GetTextSurfaceRect().origin();

    textbox_waku_->Render(box, surface_size, GetColour(), GetIsFilter());
    RenderFaces(1);

    switch (state_) {
      case State::Selection:
        for_each(selections_.begin(), selections_.end(),
                 [](std::unique_ptr<SelectionElement>& e) { e->Render(); });
        break;
      case State::Normal:
        if (name_surface_) {
          Rect r = GetNameboxWakuRect();

          if (namebox_waku_) {
            // TODO(erg): The waku needs to be adjusted to be the minimum size
            // of the window in characters
            namebox_waku_->Render(r.origin(), GetNameboxTextArea(), GetColour(),
                                  GetIsFilter());
          }

          Point insertion_point = namebox_waku_->InsertionPoint(
              r, Size(horizontal_namebox_padding_, vertical_namebox_padding_),
              name_surface_->GetSize(), namebox_centering_);
          name_surface_->RenderToScreen(
              name_surface_->GetRect(),
              Rect(insertion_point, name_surface_->GetSize()), 255);
        }

        RenderFaces(0);
        RenderKoeReplayButtons();

        text_surface_->RenderToScreen(Rect(Point(0, 0), surface_size),
                                      Rect(textOrigin, surface_size), 255);

        break;
    }
  }
}

void TextWindow::RenderFaces(int behind) {
  for (int i = 0; i < kNumFaceSlots; ++i) {
    if (face_slot_[i] && face_slot_[i]->face_surface &&
        face_slot_[i]->is_behind == behind) {
      const std::shared_ptr<const Surface>& surface =
          face_slot_[i]->face_surface;

      Rect dest(GetWindowRect().x() + face_slot_[i]->x,
                GetWindowRect().y() + face_slot_[i]->y, surface->GetSize());
      surface->RenderToScreen(surface->GetRect(), dest, 255);
    }
  }
}

void TextWindow::RenderKoeReplayButtons() {
  for (std::vector<std::pair<Point, int>>::const_iterator it =
           koe_replay_button_.begin();
       it != koe_replay_button_.end(); ++it) {
    koe_replay_info_->icon->RenderToScreen(
        Rect(Point(0, 0), koe_replay_info_->icon->GetSize()),
        Rect(GetTextSurfaceRect().origin() + it->first,
             koe_replay_info_->icon->GetSize()),
        255);
  }
}

void TextWindow::ClearWin() {
  layout_.Reset();
  font_colour_ = default_colour_;
  koe_replay_button_.clear();

  // Allocate the text window surface
  if (!text_surface_)
    text_surface_ = std::make_shared<Surface>(GetTextSurfaceSize());
  text_surface_->Fill(RGBAColour::Clear());

  name_surface_ = nullptr;
}

bool TextWindow::DisplayCharacter(const std::string& current,
                                  const std::string& rest) {
  // If this text page is already full, save some time and reject
  // early.
  if (IsFull())
    return false;

  SetVisible(true);

  if (!current.empty()) {
    int cur_codepoint = Codepoint(current);
    bool indent_after_spacing = false;

    // But if the last character was a lenticular bracket, we need to indent
    // now. See doc/notes/NamesAndIndentation.txt for more details.
    if (last_token_was_name_) {
      switch (name_mod_) {
        case NameMode::Inline:
        case NameMode::Disable:
          if (IsOpeningQuoteMark(cur_codepoint))
            indent_after_spacing = true;
          break;

        case NameMode::SeparateWindow:
          break;
      }
    }

    std::shared_ptr<IFont> font =
        system_.text().LoadFont(font_size_in_pixels());
    FontFace font_face{.font = font,
                       .is_italic = std::exchange(next_char_italic_, false)};

    // If our font is not monospaced ASCII, we aren't using
    // the recommended font so we'll try laying out the text so that
    // kerning looks better. This is the common case.
    std::optional<int> width;
    if (cur_codepoint < 127 && !font->IsMonospace())
      width = font->GetCharWidth(cur_codepoint);

    std::optional<Point> insertion_point =
        layout_.PlaceCharacter(cur_codepoint, width, rest);
    if (!insertion_point.has_value())
      return false;

    std::optional<RGBColour> shadow;
    if (system_.text().font_shadow())
      shadow = RGBAColour::Black().rgb();

    text_impl_->RenderGlyphOnto(current, font_face, font_colour_, shadow,
                                *insertion_point, text_surface_);

    if (indent_after_spacing)
      SetIndentation();
  }

  last_token_was_name_ = false;

  return true;
}

bool TextWindow::IsFull() const { return layout_.IsFull(); }

void TextWindow::KoeMarker(int id) {
  if (!koe_replay_info_) {
    koe_replay_info_.reset(new KoeReplayInfo);
    Gameexe& gexe = system_.gameexe();
    GameexeInterpretObject replay_icon(gexe("KOEREPLAYICON"));

    koe_replay_info_->icon =
        system_.graphics().GetSurfaceNamed(replay_icon("NAME").ToStr());
    std::vector<int> reppos = replay_icon("REPPOS").ToIntVec();
    if (reppos.size() == 2)
      koe_replay_info_->repos = Size(reppos[0], reppos[1]);
  }

  Point p = layout_.GetInsertionPoint() + koe_replay_info_->repos;
  koe_replay_button_.emplace_back(p, id);
}

void TextWindow::SetIndentation() {
  layout_.indent_pixels = layout_.insertion_x;
}
void TextWindow::ResetIndentation() { layout_.indent_pixels = 0; }

void TextWindow::MarkRubyBegin() { layout_.RubyBegin(); }

void TextWindow::DisplayRubyText(const std::string& utf8str) {
  if (std::optional<Rect> ruby_area = layout_.PlaceRubyText(utf8str)) {
    std::shared_ptr<IFont> ifont =
        system_.text().LoadFont(layout_.ruby_font_size);
    std::shared_ptr<SDLSurface> ruby_surface =
        text_impl_->RenderText(utf8str, FontFace(ifont), font_colour_);

    Point origin = ruby_area->origin();
    int delta_x =
        (ruby_area->size().width() - ruby_surface->GetSize().width()) / 2;
    origin += Point(delta_x, 0);

    text_surface_->blitFROMSurface(*ruby_surface, ruby_surface->GetRect(),
                                   Rect(origin, ruby_area->size()));
  }

  last_token_was_name_ = false;
}

void TextWindow::SetRGBAF(const std::vector<int>& attr) {
  ASSERTX_GE(attr.size(), 5);
  SetColour(RGBAColour(attr.at(0), attr.at(1), attr.at(2), attr.at(3)));
  SetFilter(attr.at(4));
}

void TextWindow::SetMousePosition(const Point& pos) {
  if (state_ == State::Selection) {
    for_each(selections_.begin(), selections_.end(),
             [&](std::unique_ptr<SelectionElement>& e) {
               e->SetMousePosition(pos);
             });
  }

  textbox_waku_->SetMousePosition(pos);
}

bool TextWindow::HandleMouseClick(RLMachine& machine,
                                  const Point& pos,
                                  bool pressed) {
  if (state_ == State::Selection) {
    bool found =
        std::any_of(selections_.begin(), selections_.end(),
                    [&](auto& e) { return e->HandleMouseClick(pos, pressed); });

    if (found)
      return true;
  }

  for (const auto& it : koe_replay_button_) {
    Rect r = Rect(GetTextSurfaceRect().origin() + it.first,
                  koe_replay_info_->icon->GetSize());
    if (r.Contains(pos)) {
      // We only want to actually replay the voice clip once, but we want to
      // catch both clicks.
      if (pressed)
        system_.sound().KoePlay(it.second);
      return true;
    }
  }

  if (IsVisible() && !system_.graphics().is_interface_hidden()) {
    return textbox_waku_->HandleMouseClick(machine, pos, pressed);
  }

  return false;
}

void TextWindow::StartSelection(std::vector<Selection> choices) {
  state_ = State::Selection;

  std::shared_ptr<IFont> ifont = system_.text().LoadFont(font_size_in_pixels());

  selections_.clear();
  selections_.reserve(choices.size());
  for (auto& choice : choices) {
    std::shared_ptr<Surface> normal =
        text_impl_->RenderText(choice.text, FontFace(ifont), font_colour_);

    // clone and create an inverted surface
    auto inverted = normal->Clone();
    inverted->Apply(
        [](RGBAColour c) {
          c.set_alpha(255 - c.a());
          return c;
        },
        inverted->GetRect());

    // Figure out xpos and ypos
    Point position =
        GetTextSurfaceRect().origin() + layout_.GetInsertionPoint();
    layout_.insertion_y += layout_.GetLineHeight();

    auto sel = std::make_unique<SelectionElement>(normal, inverted, position);
    sel->OnMouseover([this]() {
      constexpr int kHoverSoundEffect = 0;
      if (system_.sound().HasSe(kHoverSoundEffect))
        system_.sound().PlaySe(kHoverSoundEffect);
    });
    sel->OnSelect(std::move(choice.callback));

    selections_.emplace_back(std::move(sel));
  }
}

void TextWindow::EndSelection() {
  state_ = State::Normal;
  selections_.clear();
  ClearWin();
}
