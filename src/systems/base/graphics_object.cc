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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// -----------------------------------------------------------------------

#include "systems/base/graphics_object.h"

#include <boost/serialization/array.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "systems/base/graphics_object_data.h"
#include "systems/base/object_mutator.h"
#include "utilities/exception.h"

const Rect EMPTY_CLIP = Rect(Point(0, 0), Size(-1, -1));

const boost::shared_ptr<GraphicsObject::Impl> GraphicsObject::s_empty_impl(
    new GraphicsObject::Impl);

// -----------------------------------------------------------------------
// GraphicsObject
// -----------------------------------------------------------------------
GraphicsObject::GraphicsObject() : impl_(s_empty_impl) {}

GraphicsObject::GraphicsObject(const GraphicsObject& rhs) : impl_(rhs.impl_) {
  if (rhs.object_data_) {
    object_data_.reset(rhs.object_data_->Clone());
    object_data_->set_owned_by(*this);
  } else {
    object_data_.reset();
  }

  for (auto const& mutator : rhs.object_mutators_)
    object_mutators_.emplace_back(mutator->Clone());
}

GraphicsObject::~GraphicsObject() { DeleteObjectMutators(); }

GraphicsObject& GraphicsObject::operator=(const GraphicsObject& obj) {
  DeleteObjectMutators();
  impl_ = obj.impl_;

  if (obj.object_data_) {
    object_data_.reset(obj.object_data_->Clone());
    object_data_->set_owned_by(*this);
  } else {
    object_data_.reset();
  }

  for (auto const& mutator : obj.object_mutators_)
    object_mutators_.emplace_back(mutator->Clone());

  return *this;
}

void GraphicsObject::SetObjectData(GraphicsObjectData* obj) {
  object_data_.reset(obj);
  object_data_->set_owned_by(*this);
}

// -----------------------------------------------------------------------

void GraphicsObject::SetVisible(const int in) {
  MakeImplUnique();
  impl_->visible_ = in;
}

void GraphicsObject::SetX(const int x) {
  MakeImplUnique();
  impl_->x_ = x;
}

void GraphicsObject::SetY(const int y) {
  MakeImplUnique();
  impl_->y_ = y;
}

int GraphicsObject::GetXAdjustmentSum() const {
  return std::accumulate(impl_->adjust_x_.cbegin(), impl_->adjust_x_.cend(), 0);
}

void GraphicsObject::SetXAdjustment(int idx, int x) {
  MakeImplUnique();
  impl_->adjust_x_[idx] = x;
}

int GraphicsObject::GetYAdjustmentSum() const {
  return std::accumulate(impl_->adjust_y_.cbegin(), impl_->adjust_y_.cend(), 0);
}

void GraphicsObject::SetYAdjustment(int idx, int y) {
  MakeImplUnique();
  impl_->adjust_y_[idx] = y;
}

void GraphicsObject::SetVert(const int vert) {
  MakeImplUnique();
  impl_->whatever_adjust_vert_operates_on_ = vert;
}

void GraphicsObject::SetOriginX(const int x) {
  MakeImplUnique();
  impl_->origin_x_ = x;
}

void GraphicsObject::SetOriginY(const int y) {
  MakeImplUnique();
  impl_->origin_y_ = y;
}

void GraphicsObject::SetRepOriginX(const int x) {
  MakeImplUnique();
  impl_->rep_origin_x_ = x;
}

void GraphicsObject::SetRepOriginY(const int y) {
  MakeImplUnique();
  impl_->rep_origin_y_ = y;
}

void GraphicsObject::SetWidth(const int in) {
  MakeImplUnique();
  impl_->width_ = in;
}

void GraphicsObject::SetHeight(const int in) {
  MakeImplUnique();
  impl_->height_ = in;
}

void GraphicsObject::SetHqWidth(const int in) {
  MakeImplUnique();
  impl_->hq_width_ = in;
}

void GraphicsObject::SetHqHeight(const int in) {
  MakeImplUnique();
  impl_->hq_height_ = in;
}

float GraphicsObject::GetWidthScaleFactor() const {
  return (impl_->width_ / 100.0f) * (impl_->hq_width_ / 1000.0f);
}

float GraphicsObject::GetHeightScaleFactor() const {
  return (impl_->height_ / 100.0f) * (impl_->hq_height_ / 1000.0f);
}

void GraphicsObject::SetRotation(const int in) {
  MakeImplUnique();
  impl_->rotation_ = in;
}

int GraphicsObject::PixelWidth() const {
  // Calculate out the pixel width of the current object taking in the
  // width() scaling.
  if (has_object_data())
    return object_data_->PixelWidth(*this);
  else
    return 0;
}

int GraphicsObject::PixelHeight() const {
  if (has_object_data())
    return object_data_->PixelHeight(*this);
  else
    return 0;
}

int GraphicsObject::GetPattNo() const {
  if (GetButtonUsingOverides())
    return GetButtonPatternOverride();

  return impl_->patt_no_;
}

void GraphicsObject::SetPattNo(const int in) {
  MakeImplUnique();
  impl_->patt_no_ = in;
}

void GraphicsObject::SetMono(const int in) {
  MakeImplUnique();
  impl_->mono_ = in;
}

void GraphicsObject::SetInvert(const int in) {
  MakeImplUnique();
  impl_->invert_ = in;
}

void GraphicsObject::SetLight(const int in) {
  MakeImplUnique();
  impl_->light_ = in;
}

void GraphicsObject::SetTint(const RGBColour& colour) {
  MakeImplUnique();
  impl_->tint_ = colour;
}

void GraphicsObject::SetTintRed(const int in) {
  MakeImplUnique();
  impl_->tint_.set_red(in);
}

void GraphicsObject::SetTintGreen(const int in) {
  MakeImplUnique();
  impl_->tint_.set_green(in);
}

void GraphicsObject::SetTintBlue(const int in) {
  MakeImplUnique();
  impl_->tint_.set_blue(in);
}

void GraphicsObject::SetColour(const RGBAColour& colour) {
  MakeImplUnique();
  impl_->colour_ = colour;
}

void GraphicsObject::SetColourRed(const int in) {
  MakeImplUnique();
  impl_->colour_.set_red(in);
}

void GraphicsObject::SetColourGreen(const int in) {
  MakeImplUnique();
  impl_->colour_.set_green(in);
}

void GraphicsObject::SetColourBlue(const int in) {
  MakeImplUnique();
  impl_->colour_.set_blue(in);
}

void GraphicsObject::SetColourLevel(const int in) {
  MakeImplUnique();
  impl_->colour_.set_alpha(in);
}

void GraphicsObject::SetCompositeMode(const int in) {
  MakeImplUnique();
  impl_->composite_mode_ = in;
}

void GraphicsObject::SetScrollRateX(const int x) {
  MakeImplUnique();
  impl_->scroll_rate_x_ = x;
}

void GraphicsObject::SetScrollRateY(const int y) {
  MakeImplUnique();
  impl_->scroll_rate_y_ = y;
}

void GraphicsObject::SetZOrder(const int in) {
  MakeImplUnique();
  impl_->z_order_ = in;
}

void GraphicsObject::SetZLayer(const int in) {
  MakeImplUnique();
  impl_->z_layer_ = in;
}

void GraphicsObject::SetZDepth(const int in) {
  MakeImplUnique();
  impl_->z_depth_ = in;
}

int GraphicsObject::GetComputedAlpha() const {
  int alpha = impl_->alpha_;
  for (int i = 0; i < 8; ++i)
    alpha = (alpha * impl_->adjust_alpha_[i]) / 255;
  return alpha;
}

void GraphicsObject::SetAlpha(const int alpha) {
  MakeImplUnique();
  impl_->alpha_ = alpha;
}

void GraphicsObject::SetAlphaAdjustment(int idx, int alpha) {
  MakeImplUnique();
  impl_->adjust_alpha_[idx] = alpha;
}

void GraphicsObject::ClearClipRect() {
  MakeImplUnique();
  impl_->clip_ = EMPTY_CLIP;
}

void GraphicsObject::SetClipRect(const Rect& rect) {
  MakeImplUnique();
  impl_->clip_ = rect;
}

void GraphicsObject::ClearOwnClipRect() {
  MakeImplUnique();
  impl_->own_clip_ = EMPTY_CLIP;
}

void GraphicsObject::SetOwnClipRect(const Rect& rect) {
  MakeImplUnique();
  impl_->own_clip_ = rect;
}

GraphicsObjectData& GraphicsObject::GetObjectData() {
  if (object_data_) {
    return *object_data_;
  } else {
    throw rlvm::Exception("null object data");
  }
}

void GraphicsObject::SetWipeCopy(const int wipe_copy) {
  MakeImplUnique();
  impl_->wipe_copy_ = wipe_copy;
}

void GraphicsObject::SetTextText(const std::string& utf8str) {
  MakeImplUnique();
  impl_->text_properties_.value = utf8str;
}

const std::string& GraphicsObject::GetTextText() const {
  return impl_->text_properties_.value;
}

int GraphicsObject::GetTextSize() const {
  return impl_->text_properties_.text_size;
}

int GraphicsObject::GetTextXSpace() const {
  return impl_->text_properties_.xspace;
}

int GraphicsObject::GetTextYSpace() const {
  return impl_->text_properties_.yspace;
}

int GraphicsObject::GetTextCharCount() const {
  return impl_->text_properties_.char_count;
}

int GraphicsObject::GetTextColour() const {
  return impl_->text_properties_.colour;
}

int GraphicsObject::GetTextShadowColour() const {
  return impl_->text_properties_.shadow_colour;
}

void GraphicsObject::SetTextOps(int size,
                                int xspace,
                                int yspace,
                                int char_count,
                                int colour,
                                int shadow) {
  MakeImplUnique();

  impl_->text_properties_.text_size = size;
  impl_->text_properties_.xspace = xspace;
  impl_->text_properties_.yspace = yspace;
  impl_->text_properties_.char_count = char_count;
  impl_->text_properties_.colour = colour;
  impl_->text_properties_.shadow_colour = shadow;
}

void GraphicsObject::SetDriftOpts(int count,
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
  MakeImplUnique();

  impl_->drift_properties_.count = count;
  impl_->drift_properties_.use_animation = use_animation;
  impl_->drift_properties_.start_pattern = start_pattern;
  impl_->drift_properties_.end_pattern = end_pattern;
  impl_->drift_properties_.total_animation_time_ms = total_animation_time_ms;
  impl_->drift_properties_.yspeed = yspeed;
  impl_->drift_properties_.period = period;
  impl_->drift_properties_.amplitude = amplitude;
  impl_->drift_properties_.use_drift = use_drift;
  impl_->drift_properties_.unknown_drift_property = unknown_drift_property;
  impl_->drift_properties_.driftspeed = driftspeed;
  impl_->drift_properties_.drift_area = driftarea;
}

int GraphicsObject::GetDriftParticleCount() const {
  return impl_->drift_properties_.count;
}

int GraphicsObject::GetDriftUseAnimation() const {
  return impl_->drift_properties_.use_animation;
}

int GraphicsObject::GetDriftStartPattern() const {
  return impl_->drift_properties_.start_pattern;
}

int GraphicsObject::GetDriftEndPattern() const {
  return impl_->drift_properties_.end_pattern;
}

int GraphicsObject::GetDriftAnimationTime() const {
  return impl_->drift_properties_.total_animation_time_ms;
}

int GraphicsObject::GetDriftYSpeed() const {
  return impl_->drift_properties_.yspeed;
}

int GraphicsObject::GetDriftPeriod() const {
  return impl_->drift_properties_.period;
}

int GraphicsObject::GetDriftAmplitude() const {
  return impl_->drift_properties_.amplitude;
}

int GraphicsObject::GetDriftUseDrift() const {
  return impl_->drift_properties_.use_drift;
}

int GraphicsObject::GetDriftUnknown() const {
  return impl_->drift_properties_.unknown_drift_property;
}

int GraphicsObject::GetDriftDriftSpeed() const {
  return impl_->drift_properties_.driftspeed;
}

Rect GraphicsObject::GetDriftArea() const {
  return impl_->drift_properties_.drift_area;
}

void GraphicsObject::SetDigitValue(int value) {
  MakeImplUnique();
  impl_->digit_properties_.value = value;
}

void GraphicsObject::SetDigitOpts(int digits,
                                  int zero,
                                  int sign,
                                  int pack,
                                  int space) {
  MakeImplUnique();

  impl_->digit_properties_.digits = digits;
  impl_->digit_properties_.zero = zero;
  impl_->digit_properties_.sign = sign;
  impl_->digit_properties_.pack = pack;
  impl_->digit_properties_.space = space;
}

int GraphicsObject::GetDigitValue() const {
  return impl_->digit_properties_.value;
}

int GraphicsObject::GetDigitDigits() const {
  return impl_->digit_properties_.digits;
}

int GraphicsObject::GetDigitZero() const {
  return impl_->digit_properties_.zero;
}

int GraphicsObject::GetDigitSign() const {
  return impl_->digit_properties_.sign;
}

int GraphicsObject::GetDigitPack() const {
  return impl_->digit_properties_.pack;
}

int GraphicsObject::GetDigitSpace() const {
  return impl_->digit_properties_.space;
}

void GraphicsObject::SetButtonOpts(int action,
                                   int se,
                                   int group,
                                   int button_number) {
  MakeImplUnique();
  impl_->button_properties_.is_button = true;
  impl_->button_properties_.action = action;
  impl_->button_properties_.se = se;
  impl_->button_properties_.group = group;
  impl_->button_properties_.button_number = button_number;
}

void GraphicsObject::SetButtonState(int state) {
  MakeImplUnique();
  impl_->button_properties_.state = state;
}

int GraphicsObject::IsButton() const {
  return impl_->button_properties_.is_button;
}

int GraphicsObject::GetButtonAction() const {
  return impl_->button_properties_.action;
}

int GraphicsObject::GetButtonSe() const { return impl_->button_properties_.se; }

int GraphicsObject::GetButtonGroup() const {
  return impl_->button_properties_.group;
}

int GraphicsObject::GetButtonNumber() const {
  return impl_->button_properties_.button_number;
}

int GraphicsObject::GetButtonState() const {
  return impl_->button_properties_.state;
}

void GraphicsObject::SetButtonOverrides(int override_pattern,
                                        int override_x_offset,
                                        int override_y_offset) {
  MakeImplUnique();
  impl_->button_properties_.using_overides = true;
  impl_->button_properties_.pattern_override = override_pattern;
  impl_->button_properties_.x_offset_override = override_x_offset;
  impl_->button_properties_.y_offset_override = override_y_offset;
}

void GraphicsObject::ClearButtonOverrides() {
  MakeImplUnique();
  impl_->button_properties_.using_overides = false;
}

bool GraphicsObject::GetButtonUsingOverides() const {
  return impl_->button_properties_.using_overides;
}

int GraphicsObject::GetButtonPatternOverride() const {
  return impl_->button_properties_.pattern_override;
}

int GraphicsObject::GetButtonXOffsetOverride() const {
  return impl_->button_properties_.x_offset_override;
}

int GraphicsObject::GetButtonYOffsetOverride() const {
  return impl_->button_properties_.y_offset_override;
}

// -----------------------------------------------------------------------

void GraphicsObject::AddObjectMutator(std::unique_ptr<ObjectMutator> mutator) {
  MakeImplUnique();

  // If there's a currently running mutator that matches the incoming mutator,
  // we ignore the incoming mutator. Kud Wafter's ED relies on this behavior.
  for (std::unique_ptr<ObjectMutator>& mutator_ptr : object_mutators_) {
    if (mutator_ptr->OperationMatches(mutator->repr(), mutator->name())) {
      return;
    }
  }

  object_mutators_.emplace_back(std::move(mutator));
}

bool GraphicsObject::IsMutatorRunningMatching(int repno,
                                              const std::string& name) {
  for (auto const& mutator : object_mutators_) {
    if (mutator->OperationMatches(repno, name))
      return true;
  }

  return false;
}

void GraphicsObject::EndObjectMutatorMatching(RLMachine& machine,
                                              int repno,
                                              const std::string& name,
                                              int speedup) {
  if (speedup == 0) {
    std::vector<std::unique_ptr<ObjectMutator>>::iterator it =
        object_mutators_.begin();
    while (it != object_mutators_.end()) {
      if ((*it)->OperationMatches(repno, name)) {
        (*it)->SetToEnd(machine, *this);
        it = object_mutators_.erase(it);
      } else {
        ++it;
      }
    }
  } else if (speedup == 1) {
    // This is explicitly a noop.
  } else {
    std::cerr << "Warning: We only do immediate endings in "
              << "EndObjectMutatorMatching(). Unsupported speedup " << speedup
              << std::endl;
  }
}

std::vector<std::string> GraphicsObject::GetMutatorNames() const {
  std::vector<std::string> names;

  names.reserve(object_mutators_.size());
  for (auto& mutator : object_mutators_) {
    std::ostringstream oss;
    oss << mutator->name();
    if (mutator->repr() != -1)
      oss << "/" << mutator->repr();
    names.push_back(oss.str());
  }

  return names;
}

void GraphicsObject::MakeImplUnique() {
  if (!impl_.unique()) {
    impl_.reset(new Impl(*impl_));
  }
}

void GraphicsObject::DeleteObjectMutators() { object_mutators_.clear(); }

void GraphicsObject::Render(int objNum, const GraphicsObject* parent) {
  if (object_data_ && Param().visible()) {
    object_data_->Render(*this, parent);
  }
}

void GraphicsObject::FreeObjectData() {
  object_data_.reset();
  DeleteObjectMutators();
}

void GraphicsObject::InitializeParams() {
  impl_ = s_empty_impl;
  param_ = ParameterManager();
  DeleteObjectMutators();
}

void GraphicsObject::FreeDataAndInitializeParams() {
  object_data_.reset();
  impl_ = s_empty_impl;
  param_ = ParameterManager();
  DeleteObjectMutators();
}

void GraphicsObject::Execute(RLMachine& machine) {
  if (object_data_) {
    object_data_->Execute(machine);
  }

  // Run each mutator. If it returns true, remove it.
  std::vector<std::unique_ptr<ObjectMutator>>::iterator it =
      object_mutators_.begin();
  while (it != object_mutators_.end()) {
    if ((**it)(machine, *this)) {
      it = object_mutators_.erase(it);
    } else {
      ++it;
    }
  }
}

template <class Archive>
void GraphicsObject::serialize(Archive& ar, unsigned int version) {
  ar & impl_ & object_data_;
}

// -----------------------------------------------------------------------

template void GraphicsObject::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void GraphicsObject::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);

// -----------------------------------------------------------------------
// GraphicsObject::Impl
// -----------------------------------------------------------------------
GraphicsObject::Impl::Impl()
    : visible_(false),
      x_(0),
      y_(0),
      adjust_x_(),
      adjust_y_(),
      whatever_adjust_vert_operates_on_(0),
      origin_x_(0),
      origin_y_(0),
      rep_origin_x_(0),
      rep_origin_y_(0),

      // Width and height are percentages
      width_(100),
      height_(100),
      hq_width_(1000),
      hq_height_(1000),
      rotation_(0),
      patt_no_(0),
      alpha_(255),
      adjust_alpha_(),
      clip_(EMPTY_CLIP),
      own_clip_(EMPTY_CLIP),
      mono_(0),
      invert_(0),
      light_(0),
      // Do the rest later.
      tint_(RGBColour::Black()),
      colour_(RGBAColour::Clear()),
      composite_mode_(0),
      scroll_rate_x_(0),
      scroll_rate_y_(0),
      z_order_(0),
      z_layer_(0),
      z_depth_(0),
      wipe_copy_(0) {
  std::fill(adjust_alpha_.begin(), adjust_alpha_.end(), 255);
}

GraphicsObject::Impl::Impl(const Impl& rhs)
    : visible_(rhs.visible_),
      x_(rhs.x_),
      y_(rhs.y_),
      adjust_x_(rhs.adjust_x_),
      adjust_y_(rhs.adjust_y_),
      whatever_adjust_vert_operates_on_(rhs.whatever_adjust_vert_operates_on_),
      origin_x_(rhs.origin_x_),
      origin_y_(rhs.origin_y_),
      rep_origin_x_(rhs.rep_origin_x_),
      rep_origin_y_(rhs.rep_origin_y_),
      width_(rhs.width_),
      height_(rhs.height_),
      hq_width_(rhs.hq_width_),
      hq_height_(rhs.hq_height_),
      rotation_(rhs.rotation_),
      patt_no_(rhs.patt_no_),
      alpha_(rhs.alpha_),
      adjust_alpha_(rhs.adjust_alpha_),
      clip_(rhs.clip_),
      own_clip_(rhs.own_clip_),
      mono_(rhs.mono_),
      invert_(rhs.invert_),
      light_(rhs.light_),
      tint_(rhs.tint_),
      colour_(rhs.colour_),
      composite_mode_(rhs.composite_mode_),
      scroll_rate_x_(rhs.scroll_rate_x_),
      scroll_rate_y_(rhs.scroll_rate_y_),
      z_order_(rhs.z_order_),
      z_layer_(rhs.z_layer_),
      z_depth_(rhs.z_depth_),
      wipe_copy_(0) {
  text_properties_ = rhs.text_properties_;
  drift_properties_ = rhs.drift_properties_;
  digit_properties_ = rhs.digit_properties_;
  button_properties_ = rhs.button_properties_;
}

GraphicsObject::Impl::~Impl() {}

GraphicsObject::Impl& GraphicsObject::Impl::operator=(
    const GraphicsObject::Impl& rhs) {
  if (this != &rhs) {
    visible_ = rhs.visible_;
    x_ = rhs.x_;
    y_ = rhs.y_;
    adjust_x_ = rhs.adjust_x_;
    adjust_y_ = rhs.adjust_y_;
    adjust_alpha_ = rhs.adjust_alpha_;

    whatever_adjust_vert_operates_on_ = rhs.whatever_adjust_vert_operates_on_;
    origin_x_ = rhs.origin_x_;
    origin_y_ = rhs.origin_y_;
    rep_origin_x_ = rhs.rep_origin_x_;
    rep_origin_y_ = rhs.rep_origin_y_;
    width_ = rhs.width_;
    height_ = rhs.height_;
    hq_width_ = rhs.hq_width_;
    hq_height_ = rhs.hq_height_;
    rotation_ = rhs.rotation_;

    patt_no_ = rhs.patt_no_;
    alpha_ = rhs.alpha_;
    clip_ = rhs.clip_;
    own_clip_ = rhs.own_clip_;
    mono_ = rhs.mono_;
    invert_ = rhs.invert_;
    light_ = rhs.light_;
    tint_ = rhs.tint_;

    colour_ = rhs.colour_;

    composite_mode_ = rhs.composite_mode_;
    scroll_rate_x_ = rhs.scroll_rate_x_;
    scroll_rate_y_ = rhs.scroll_rate_y_;
    z_order_ = rhs.z_order_;
    z_layer_ = rhs.z_layer_;
    z_depth_ = rhs.z_depth_;

    text_properties_ = rhs.text_properties_;
    drift_properties_ = rhs.drift_properties_;
    digit_properties_ = rhs.digit_properties_;
    button_properties_ = rhs.button_properties_;

    wipe_copy_ = rhs.wipe_copy_;
  }

  return *this;
}

// boost::serialization support
template <class Archive>
void GraphicsObject::Impl::serialize(Archive& ar, unsigned int version) {
  ar & visible_ & x_ & y_ & whatever_adjust_vert_operates_on_ & origin_x_ &
      origin_y_ & rep_origin_x_ & rep_origin_y_ & width_ & height_ & rotation_ &
      patt_no_ & alpha_ & clip_ & mono_ & invert_ & tint_ & colour_ &
      composite_mode_ & text_properties_ & wipe_copy_;

  if (version > 0) {
    ar & drift_properties_;
  }

  if (version > 1) {
    ar & digit_properties_;
  }

  if (version > 2) {
    ar & adjust_x_ & adjust_y_ & adjust_alpha_;
  }

  if (version > 3) {
    ar & hq_width_ & hq_height_ & button_properties_;
  }

  if (version > 4) {
    ar & own_clip_;
  }

  if (version > 5) {
    ar & z_order_ & z_layer_ & z_depth_;
  }

  if (version < 7) {
    // Before version 7, tint and colour were set incorrectly. Therefore the
    // vast majority of values in save games were set incorrectly. Oops. Set to
    // the default here.
    tint_ = RGBColour::Black();
    colour_ = RGBAColour::Clear();
  }
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void GraphicsObject::Impl::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version);

template void GraphicsObject::Impl::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
