// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
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

#include "core/frame_counter.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>  // std::ignore

#include "utilities/clock.hpp"

// -----------------------------------------------------------------------
// class FrameCounter
FrameCounter::FrameCounter(std::shared_ptr<Clock> clock,
                           int frame_min,
                           int frame_max,
                           int milliseconds)
    : clock_(clock),
      value_(static_cast<float>(frame_min)),
      min_value_(frame_min),
      max_value_(frame_max),
      is_active_(true),
      start_time_(clock_->GetTicks()),
      total_time_(std::chrono::milliseconds(std::max(0, milliseconds))) {
  if (milliseconds == 0 || frame_min == frame_max) {
    value_ = static_cast<float>(frame_max);
    is_active_ = false;
  }
}

FrameCounter::~FrameCounter() = default;

void FrameCounter::BeginTimer(std::chrono::milliseconds delay) {
  start_time_ = clock_->GetTicks() + delay;
  is_active_ = true;
}

void FrameCounter::EndTimer() {
  // Keep current value_ updated
  std::ignore = ReadFrame();
  is_active_ = false;
}

double FrameCounter::ComputeNormalizedTime() const {
  if (!is_active_) {
    // If not active, we act as if it's fully done
    return 1.0;
  }
  if (total_time_.count() <= 0) {
    // Avoid divide-by-zero; treat as done
    return 1.0;
  }

  auto now = clock_->GetTicks();
  auto elapsed = now - start_time_;
  double fraction = static_cast<double>(elapsed.count()) /
                    static_cast<double>(total_time_.count());

  return fraction;
}

double FrameCounter::ClampFractionToOneShot(double fraction) {
  return std::clamp(fraction, 0.0, 1.0);
}

// -----------------------------------------------------------------------
// class SimpleFrameCounter
float SimpleFrameCounter::ReadFrame() {
  double fraction = ComputeNormalizedTime();
  if (fraction >= 1.0)
    is_active_ = false;

  fraction = ClampFractionToOneShot(fraction);

  double range = double(max_value_ - min_value_);
  double current = double(min_value_) + (fraction * range);

  return value_ = static_cast<float>(current);
}

std::unique_ptr<FrameCounter> SimpleFrameCounter::Clone() const {
  return std::make_unique<SimpleFrameCounter>(*this);
}

// -----------------------------------------------------------------------
// class LoopFrameCounter
float LoopFrameCounter::ReadFrame() {
  double fraction = ComputeNormalizedTime();
  if (!IsActive() || fraction <= 0.0)
    return value_;

  double integralPart = std::floor(fraction);
  double fracPart = fraction - integralPart;

  double range = static_cast<double>(max_value_ - min_value_);
  double current = static_cast<double>(min_value_) + (fracPart * range);

  return value_ = static_cast<float>(current);
}

std::unique_ptr<FrameCounter> LoopFrameCounter::Clone() const {
  return std::make_unique<LoopFrameCounter>(*this);
}

// -----------------------------------------------------------------------
// class TurnFrameCounter
float TurnFrameCounter::ReadFrame() {
  double fraction = ComputeNormalizedTime();
  if (!IsActive() || fraction <= 0.0)
    return value_;

  double range = static_cast<double>(max_value_ - min_value_);
  if (range == 0.0) {
    return min_value_;
  }

  double cycle = std::fmod(fraction, 2.0);
  // We want a "triangle wave" from 0..1..0 across cycle=0..1
  // But simpler approach: if cycle < 0.5 => ascend, else descend
  // Or more general formula for a symmetrical triangle wave:
  // wave = 1.0 - |1.0 - cycle|
  double wave = 1.0 - std::fabs(1.0 - cycle);

  // wave now in [0..1], 0.0 at extremes of the cycle, 1.0 at mid.
  double current = double(min_value_) + range * wave;
  return value_ = static_cast<float>(current);
}

std::unique_ptr<FrameCounter> TurnFrameCounter::Clone() const {
  return std::make_unique<TurnFrameCounter>(*this);
}

// -----------------------------------------------------------------------
// class AcceleratingFrameCounter
float AcceleratingFrameCounter::ReadFrame() {
  double fraction = ComputeNormalizedTime();
  if (fraction >= 1.0)
    is_active_ = false;

  fraction = ClampFractionToOneShot(fraction);

  double accelerated = fraction * fraction;

  double range = double(max_value_ - min_value_);
  double current = double(min_value_) + (range * accelerated);

  return value_ = static_cast<float>(current);
}

std::unique_ptr<FrameCounter> AcceleratingFrameCounter::Clone() const {
  return std::make_unique<AcceleratingFrameCounter>(*this);
}

// -----------------------------------------------------------------------
// class DeceleratingFrameCounter
float DeceleratingFrameCounter::ReadFrame() {
  double fraction = ComputeNormalizedTime();
  if (fraction >= 1.0)
    is_active_ = false;

  fraction = ClampFractionToOneShot(fraction);

  fraction = 1 - fraction;
  double decelerated = 1 - fraction * fraction;

  double range = double(max_value_ - min_value_);
  double current = double(min_value_) + (range * decelerated);

  return value_ = static_cast<float>(current);
}

std::unique_ptr<FrameCounter> DeceleratingFrameCounter::Clone() const {
  return std::make_unique<DeceleratingFrameCounter>(*this);
}
