// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#pragma once

#include "object/bst.hpp"
#include "object/properties.hpp"
#include "utilities/mpl.hpp"

#include <functional>
#include <numeric>
#include <utility>

#include <boost/serialization/array.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

extern const Rect EMPTY_RECT;

class ParameterManager {
 public:
  ParameterManager();
  ~ParameterManager() = default;

  void Set(ObjectProperty property, std::any value) {
    bst_.Set(static_cast<int>(property), std::move(value));
  }

  template <ObjectProperty property>
  auto Get() const {
    constexpr int param_id = static_cast<int>(property);
    if constexpr (param_id < 0) {
      static_assert(false, "Invalid parameter ID");
    }

    using result_t = typename GetNthType<static_cast<size_t>(property),
                                         ObjectPropertyType>::type;
    try {
      return std::any_cast<result_t>(bst_.Get(param_id));
    } catch (std::bad_any_cast& e) {
      throw std::logic_error("ParameterManager: Parameter type mismatch (" +
                             std::to_string(param_id) + ")\n" + e.what());
    }
  }

  std::any Get(ObjectProperty property) const {
    return bst_.Get(static_cast<int>(property));
  }

  // Methods moved from GraphicsObject

  int visible() const {
    return static_cast<int>(Get<ObjectProperty::IsVisible>());
  }
  void SetVisible(const int in) {
    Set(ObjectProperty::IsVisible, static_cast<bool>(in));
  }

  int x() const { return Get<ObjectProperty::PositionX>(); }
  void SetX(const int x) { Set(ObjectProperty::PositionX, x); }

  int y() const { return Get<ObjectProperty::PositionY>(); }
  void SetY(const int y) { Set(ObjectProperty::PositionY, y); }

  int x_adjustment(int idx) const {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsX>();
    return arr[idx];
  }
  int GetXAdjustmentSum() const {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsX>();
    return std::accumulate(arr.cbegin(), arr.cend(), 0);
  }
  void SetXAdjustment(int idx, int x) {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsX>();
    arr[idx] = x;
    Set(ObjectProperty::AdjustmentOffsetsX, std::move(arr));
  }

  int y_adjustment(int idx) const {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsY>();
    return arr[idx];
  }
  int GetYAdjustmentSum() const {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsY>();
    return std::accumulate(arr.cbegin(), arr.cend(), 0);
  }
  void SetYAdjustment(int idx, int y) {
    auto arr = Get<ObjectProperty::AdjustmentOffsetsY>();
    arr[idx] = y;
    Set(ObjectProperty::AdjustmentOffsetsY, std::move(arr));
  }

  int vert() const { return Get<ObjectProperty::AdjustmentVertical>(); }
  void SetVert(int in) { Set(ObjectProperty::AdjustmentVertical, in); }

  int origin_x() const { return Get<ObjectProperty::OriginX>(); }
  void SetOriginX(int in) { Set(ObjectProperty::OriginX, in); }

  int origin_y() const { return Get<ObjectProperty::OriginY>(); }
  void SetOriginY(int in) { Set(ObjectProperty::OriginY, in); }

  int rep_origin_x() const { return Get<ObjectProperty::RepetitionOriginX>(); }
  void SetRepOriginX(int in) { Set(ObjectProperty::RepetitionOriginX, in); }

  int rep_origin_y() const { return Get<ObjectProperty::RepetitionOriginY>(); }
  void SetRepOriginY(int in) { Set(ObjectProperty::RepetitionOriginY, in); }

  // Note: width/height are object scale percentages.
  int width() const { return Get<ObjectProperty::WidthPercent>(); }
  void SetWidth(const int in) { Set(ObjectProperty::WidthPercent, in); }
  int height() const { return Get<ObjectProperty::HeightPercent>(); }
  void SetHeight(const int in) { Set(ObjectProperty::HeightPercent, in); }

  // Note: width/height are object scale factors out of 1000.
  int hq_width() const {
    return Get<ObjectProperty::HighQualityWidthPercent>();
  }
  void SetHqWidth(const int in) {
    Set(ObjectProperty::HighQualityWidthPercent, in);
  }
  int hq_height() const {
    return Get<ObjectProperty::HighQualityHeightPercent>();
  }
  void SetHqHeight(const int in) {
    Set(ObjectProperty::HighQualityHeightPercent, in);
  }

  float GetWidthScaleFactor() const {
    return (static_cast<float>(width()) / 100.0f) *
           (static_cast<float>(hq_width()) / 1000.0f);
  }
  float GetHeightScaleFactor() const {
    return (static_cast<float>(height()) / 100.0f) *
           (static_cast<float>(hq_height()) / 1000.0f);
  }

  int rotation() const { return Get<ObjectProperty::RotationDiv10>(); }
  void SetRotation(const int in) { Set(ObjectProperty::RotationDiv10, in); }

  int GetPattNo() const {
    auto button = Get<ObjectProperty::ButtonProperties>();
    if (button.using_overides)
      return button.pattern_override;

    return Get<ObjectProperty::PatternNumber>();
  }
  void SetPattNo(const int in) { Set(ObjectProperty::PatternNumber, in); }

  int mono() const { return Get<ObjectProperty::MonochromeTransform>(); }
  void SetMono(const int in) { Set(ObjectProperty::MonochromeTransform, in); }

  int invert() const { return Get<ObjectProperty::InvertTransform>(); }
  void SetInvert(const int in) { Set(ObjectProperty::InvertTransform, in); }

  int light() const { return Get<ObjectProperty::LightLevel>(); }
  void SetLight(const int in) { Set(ObjectProperty::LightLevel, in); }

  RGBColour tint() const { return Get<ObjectProperty::TintColour>(); }
  int tint_red() const { return tint().r(); }
  int tint_green() const { return tint().g(); }
  int tint_blue() const { return tint().b(); }
  void SetTint(const RGBColour& in) { Set(ObjectProperty::TintColour, in); }
  void SetTintRed(const int r) {
    auto in = tint();
    in.set_red(r);
    Set(ObjectProperty::TintColour, std::move(in));
  }
  void SetTintGreen(const int g) {
    auto in = tint();
    in.set_green(g);
    Set(ObjectProperty::TintColour, std::move(in));
  }
  void SetTintBlue(const int b) {
    auto in = tint();
    in.set_blue(b);
    Set(ObjectProperty::TintColour, std::move(in));
  }

  RGBAColour colour() const { return Get<ObjectProperty::BlendColour>(); }
  int colour_red() const { return colour().r(); }
  int colour_green() const { return colour().g(); }
  int colour_blue() const { return colour().b(); }
  int colour_level() const { return colour().a(); }
  void SetColour(const RGBAColour& in) { Set(ObjectProperty::BlendColour, in); }
  void SetColourRed(const int in) {
    auto col = colour();
    col.set_red(in);
    SetColour(col);
  }
  void SetColourGreen(const int in) {
    auto col = colour();
    col.set_green(in);
    SetColour(col);
  }
  void SetColourBlue(const int in) {
    auto col = colour();
    col.set_blue(in);
    SetColour(col);
  }
  void SetColourLevel(const int in) {
    auto col = colour();
    col.set_alpha(in);
    SetColour(col);
  }

  int composite_mode() const { return Get<ObjectProperty::CompositeMode>(); }
  void SetCompositeMode(int in) { Set(ObjectProperty::CompositeMode, in); }

  int scroll_rate_x() const { return Get<ObjectProperty::ScrollRateX>(); }
  void SetScrollRateX(int in) { Set(ObjectProperty::ScrollRateX, in); }

  int scroll_rate_y() const { return Get<ObjectProperty::ScrollRateY>(); }
  void SetScrollRateY(int in) { Set(ObjectProperty::ScrollRateY, in); }

  int z_order() const { return Get<ObjectProperty::ZOrder>(); }
  void SetZOrder(const int in) { Set(ObjectProperty::ZOrder, in); }
  int z_layer() const { return Get<ObjectProperty::ZLayer>(); }
  void SetZLayer(const int in) { Set(ObjectProperty::ZLayer, in); }
  int z_depth() const { return Get<ObjectProperty::ZDepth>(); }
  void SetZDepth(const int in) { Set(ObjectProperty::ZDepth, in); }

  int GetComputedAlpha() const {
    auto alpha = raw_alpha();
    const auto alpha_adj = alpha_adjustment();
    for (const auto it : alpha_adj)
      alpha = (alpha * it) / 255;
    return alpha;
  }
  int raw_alpha() const { return Get<ObjectProperty::AlphaSource>(); }
  void SetAlpha(const int in) { Set(ObjectProperty::AlphaSource, in); }

  std::array<int, 8> alpha_adjustment() const {
    return Get<ObjectProperty::AdjustmentAlphas>();
  }
  int alpha_adjustment(int idx) const { return alpha_adjustment()[idx]; }
  void SetAlphaAdjustment(int idx, int alpha) {
    auto adj = alpha_adjustment();
    adj[idx] = alpha;
    Set(ObjectProperty::AdjustmentAlphas, std::move(adj));
  }

  Rect clip_rect() const { return Get<ObjectProperty::ClippingRegion>(); }
  bool has_clip_rect() const {
    const auto clip = clip_rect();
    return clip.width() >= 0 || clip.height() >= 0;
  }
  void ClearClipRect() { SetClipRect(EMPTY_RECT); }
  void SetClipRect(const Rect& in) { Set(ObjectProperty::ClippingRegion, in); }

  Rect own_clip_rect() const {
    return Get<ObjectProperty::OwnSpaceClippingRegion>();
  }
  bool has_own_clip_rect() const {
    const auto clip = own_clip_rect();
    return clip.width() >= 0 || clip.height() >= 0;
  }
  void ClearOwnClipRect() { SetOwnClipRect(EMPTY_RECT); }
  void SetOwnClipRect(const Rect& in) {
    Set(ObjectProperty::OwnSpaceClippingRegion, in);
  }

  void SetWipeCopy(int in) { Set(ObjectProperty::WipeCopy, in); }
  int wipe_copy() const { return Get<ObjectProperty::WipeCopy>(); }

  // TextProperties accessors
  TextProperties TextProperty() const {
    return Get<ObjectProperty::TextProperties>();
  }
  void SetTextProperty(const TextProperties& in) {
    Set(ObjectProperty::TextProperties, in);
  }
  void SetTextText(const std::string& utf8str) {
    auto text = TextProperty();
    text.value = utf8str;
    SetTextProperty(text);
  }
  std::string GetTextText() const { return TextProperty().value; }

  void SetTextOps(int size,
                  int xspace,
                  int yspace,
                  int char_count,
                  int colour,
                  int shadow) {
    auto text = TextProperty();
    text.text_size = size;
    text.xspace = xspace;
    text.yspace = yspace;
    text.char_count = char_count;
    text.colour = colour;
    text.shadow_colour = shadow;
    SetTextProperty(text);
  }
  int GetTextSize() const { return TextProperty().text_size; }
  int GetTextXSpace() const { return TextProperty().xspace; }
  int GetTextYSpace() const { return TextProperty().yspace; }
  int GetTextCharCount() const { return TextProperty().char_count; }
  int GetTextColour() const { return TextProperty().colour; }
  int GetTextShadowColour() const { return TextProperty().shadow_colour; }

  // DriftProperties accessors
  DriftProperties DriftProperty() const {
    return Get<ObjectProperty::DriftProperties>();
  }
  void SetDriftProperty(const DriftProperties& in) {
    Set(ObjectProperty::DriftProperties, in);
  }
  void SetDriftOpts(int count,
                    int use_animation,
                    int start_pattern,
                    int end_pattern,
                    int total_animation_time_ms,
                    int yspeed,
                    int period,
                    int amplitude,
                    int use_drift,
                    int unknown_drift_property,
                    int driftspeed,
                    Rect driftarea) {
    DriftProperties drift;
    drift.count = count;
    drift.use_animation = use_animation;
    drift.start_pattern = start_pattern;
    drift.end_pattern = end_pattern;
    drift.total_animation_time_ms = total_animation_time_ms;
    drift.yspeed = yspeed;
    drift.period = period;
    drift.amplitude = amplitude;
    drift.use_drift = use_drift;
    drift.unknown_drift_property = unknown_drift_property;
    drift.driftspeed = driftspeed;
    drift.drift_area = driftarea;
    SetDriftProperty(drift);
  }
  int GetDriftParticleCount() const { return DriftProperty().count; }
  int GetDriftUseAnimation() const { return DriftProperty().use_animation; }
  int GetDriftStartPattern() const { return DriftProperty().start_pattern; }
  int GetDriftEndPattern() const { return DriftProperty().end_pattern; }
  int GetDriftAnimationTime() const {
    return DriftProperty().total_animation_time_ms;
  }
  int GetDriftYSpeed() const { return DriftProperty().yspeed; }
  int GetDriftPeriod() const { return DriftProperty().period; }
  int GetDriftAmplitude() const { return DriftProperty().amplitude; }
  int GetDriftUseDrift() const { return DriftProperty().use_drift; }
  int GetDriftUnknown() const { return DriftProperty().unknown_drift_property; }
  int GetDriftDriftSpeed() const { return DriftProperty().driftspeed; }
  Rect GetDriftArea() const { return DriftProperty().drift_area; }

  // DigitProperties accessors
  DigitProperties DigitProperty() const {
    return Get<ObjectProperty::DigitProperties>();
  }
  void SetDigitProperty(const DigitProperties& in) {
    Set(ObjectProperty::DigitProperties, in);
  }
  void SetDigitValue(int value) {
    auto digit = DigitProperty();
    digit.value = value;
    SetDigitProperty(digit);
  }
  void SetDigitOpts(int digits, int zero, int sign, int pack, int space) {
    auto digit = DigitProperty();
    digit.digits = digits;
    digit.zero = zero;
    digit.sign = sign;
    digit.pack = pack;
    digit.space = space;
    SetDigitProperty(digit);
  }
  int GetDigitValue() const { return DigitProperty().value; }
  int GetDigitDigits() const { return DigitProperty().digits; }
  int GetDigitZero() const { return DigitProperty().zero; }
  int GetDigitSign() const { return DigitProperty().sign; }
  int GetDigitPack() const { return DigitProperty().pack; }
  int GetDigitSpace() const { return DigitProperty().space; }

  // ButtonProperties accessors
  ButtonProperties ButtonProperty() const {
    return Get<ObjectProperty::ButtonProperties>();
  }
  void SetButtonProperty(const ButtonProperties& in) {
    Set(ObjectProperty::ButtonProperties, in);
  }
  void SetButtonOpts(int action, int se, int group, int button_number) {
    auto button = ButtonProperty();
    button.is_button = 1;
    button.action = action;
    button.se = se;
    button.group = group;
    button.button_number = button_number;
    SetButtonProperty(button);
  }
  void SetButtonState(int state) {
    auto button = ButtonProperty();
    button.state = state;
    SetButtonProperty(button);
  }
  int IsButton() const { return ButtonProperty().is_button; }
  int GetButtonAction() const { return ButtonProperty().action; }
  int GetButtonSe() const { return ButtonProperty().se; }
  int GetButtonGroup() const { return ButtonProperty().group; }
  int GetButtonNumber() const { return ButtonProperty().button_number; }
  int GetButtonState() const { return ButtonProperty().state; }
  void SetButtonOverrides(int override_pattern,
                          int override_x_offset,
                          int override_y_offset) {
    auto button = ButtonProperty();
    button.using_overides = true;
    button.pattern_override = override_pattern;
    button.x_offset_override = override_x_offset;
    button.y_offset_override = override_y_offset;
    SetButtonProperty(button);
  }
  void ClearButtonOverrides() {
    auto button = ButtonProperty();
    button.using_overides = false;
    SetButtonProperty(button);
  }
  bool GetButtonUsingOverides() const {
    return ButtonProperty().using_overides;
  }
  int GetButtonPatternOverride() const {
    return ButtonProperty().pattern_override;
  }
  int GetButtonXOffsetOverride() const {
    return ButtonProperty().x_offset_override;
  }
  int GetButtonYOffsetOverride() const {
    return ButtonProperty().y_offset_override;
  }

 private:
  Scapegoat bst_;

  // boost serialization support
  friend class boost::serialization::access;
  BOOST_SERIALIZATION_SPLIT_MEMBER();

  template <class Archive, size_t idx>
  void load_one_impl(Archive& ar) {
    using T = typename GetNthType<idx, ObjectPropertyType>::type;
    T temp;
    ar & temp;
    Set(static_cast<ObjectProperty>(idx), std::move(temp));
  }
  template <class Archive, size_t... I>
  auto load_all_impl(Archive& ar, std::index_sequence<I...> /*unused*/) {
    (load_one_impl<Archive, I>(ar), ...);
  }
  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    load_all_impl(ar, std::make_index_sequence<static_cast<size_t>(
                          ObjectProperty::TOTAL_COUNT)>{});
  }

  template <class Archive, size_t... I>
  void save_all_impl(Archive& ar, std::index_sequence<I...> /*unused*/) const {
    ((ar & Get<static_cast<ObjectProperty>(I)>()), ...);
  }
  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    save_all_impl(ar, std::make_index_sequence<static_cast<size_t>(
                          ObjectProperty::TOTAL_COUNT)>{});
  }
};

template <ObjectProperty p>
auto CreateGetter() {
  using property_type =
      typename GetNthType<static_cast<size_t>(p), ObjectPropertyType>::type;

  if constexpr (std::is_same_v<property_type, std::array<int, 8>>) {  // repno
    return static_cast<std::function<int(const ParameterManager&, int)>>(
        [](const ParameterManager& param, int repno) -> int {
          auto array = param.template Get<p>();
          return array[repno];
        });
  } else if constexpr (std::is_same_v<property_type, int> ||
                       std::is_same_v<property_type, bool>) {
    return static_cast<std::function<int(const ParameterManager&)>>(
        [](const ParameterManager& param) -> int {
          return param.template Get<p>();
        });
  } else {
    static_assert(false, "Unsupported type.");
  }
}

template <ObjectProperty p>
auto CreateSetter() {
  using property_type =
      typename GetNthType<static_cast<size_t>(p), ObjectPropertyType>::type;

  if constexpr (std::is_same_v<property_type, std::array<int, 8>>) {  // repno
    return static_cast<std::function<void(ParameterManager&, int, int)>>(
        [](ParameterManager& param, int repno, int value) {
          auto array = param.template Get<p>();
          array[repno] = value;
          param.Set(p, std::move(array));
        });
  } else if constexpr (std::is_same_v<property_type, int> ||
                       std::is_same_v<property_type, bool>) {
    return static_cast<std::function<void(ParameterManager&, int)>>(
        [](ParameterManager& param, int value) {
          param.Set(p, static_cast<property_type>(value));
        });
  } else {
    static_assert(false, "Unsupported type.");
  }
}
