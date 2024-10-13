// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "systems/base/object_mutator.h"

#include "machine/rlmachine.h"
#include "object/parameter_manager.h"
#include "systems/base/event_system.h"
#include "systems/base/graphics_object.h"
#include "systems/base/graphics_object_data.h"
#include "systems/base/graphics_system.h"
#include "systems/base/parent_graphics_object_data.h"
#include "systems/base/system.h"

#include <functional>
#include <iostream>

ObjectMutator::ObjectMutator(int repr,
                             const std::string& name,
                             int creation_time,
                             int duration_time,
                             int delay,
                             int type)
    : ObjectMutator(repr,
                    name,
                    creation_time,
                    duration_time,
                    delay,
                    static_cast<InterpolationMode>(type)) {}

ObjectMutator::ObjectMutator(int repr,
                             const std::string& name,
                             int creation_time,
                             int duration_time,
                             int delay,
                             InterpolationMode type)
    : repr_(repr),
      name_(name),
      creation_time_(creation_time),
      duration_time_(duration_time),
      delay_(delay),
      type_(type) {}

bool ObjectMutator::operator()(RLMachine& machine, GraphicsObject& object) {
  unsigned int ticks = machine.system().event().GetTicks();
  if (ticks > (creation_time_ + delay_)) {
    PerformSetting(machine, object);
    machine.system().graphics().mark_object_state_as_dirty();
  }
  return ticks > (creation_time_ + delay_ + duration_time_);
}

bool ObjectMutator::OperationMatches(int repr, const std::string& name) const {
  return repr_ == repr && name_ == name;
}

int ObjectMutator::GetValueForTime(RLMachine& machine, int start, int end) {
  unsigned int ticks = machine.system().event().GetTicks();
  if (ticks < (creation_time_ + delay_)) {
    return start;
  } else if (ticks < (creation_time_ + delay_ + duration_time_)) {
    return InterpolateBetween(
        InterpolationRange(creation_time_ + delay_, ticks,
                           creation_time_ + delay_ + duration_time_),
        Range(start, end), type_);
  } else {
    return end;
  }
}

// -----------------------------------------------------------------------

OneIntObjectMutator::OneIntObjectMutator(const std::string& name,
                                         int creation_time,
                                         int duration_time,
                                         int delay,
                                         int type,
                                         int start_value,
                                         int target_value,
                                         Setter setter)
    : ObjectMutator(-1, name, creation_time, duration_time, delay, type),
      startval_(start_value),
      endval_(target_value),
      setter_(setter) {}

OneIntObjectMutator::~OneIntObjectMutator() {}

void OneIntObjectMutator::SetToEnd(RLMachine& machine, GraphicsObject& object) {
  std::invoke(setter_, object.Param(), endval_);
}

std::unique_ptr<ObjectMutator> OneIntObjectMutator::Clone() const {
  return std::make_unique<OneIntObjectMutator>(*this);
}

void OneIntObjectMutator::PerformSetting(RLMachine& machine,
                                         GraphicsObject& object) {
  int value = GetValueForTime(machine, startval_, endval_);
  std::invoke(setter_, object.Param(), value);
}

// -----------------------------------------------------------------------

RepnoIntObjectMutator::RepnoIntObjectMutator(const std::string& name,
                                             int creation_time,
                                             int duration_time,
                                             int delay,
                                             int type,
                                             int repno,
                                             int start_value,
                                             int target_value,
                                             Setter setter)
    : ObjectMutator(repno, name, creation_time, duration_time, delay, type),
      repno_(repno),
      startval_(start_value),
      endval_(target_value),
      setter_(setter) {}

RepnoIntObjectMutator::~RepnoIntObjectMutator() {}

void RepnoIntObjectMutator::SetToEnd(RLMachine& machine,
                                     GraphicsObject& object) {
  std::invoke(setter_, object.Param(), repno_, endval_);
}

std::unique_ptr<ObjectMutator> RepnoIntObjectMutator::Clone() const {
  return std::make_unique<RepnoIntObjectMutator>(*this);
}

void RepnoIntObjectMutator::PerformSetting(RLMachine& machine,
                                           GraphicsObject& object) {
  int value = GetValueForTime(machine, startval_, endval_);
  std::invoke(setter_, object.Param(), repno_, value);
}

// -----------------------------------------------------------------------

TwoIntObjectMutator::TwoIntObjectMutator(const std::string& name,
                                         int creation_time,
                                         int duration_time,
                                         int delay,
                                         int type,
                                         int start_one,
                                         int target_one,
                                         Setter setter_one,
                                         int start_two,
                                         int target_two,
                                         Setter setter_two)
    : ObjectMutator(-1, name, creation_time, duration_time, delay, type),
      startval_one_(start_one),
      endval_one_(target_one),
      setter_one_(setter_one),
      startval_two_(start_two),
      endval_two_(target_two),
      setter_two_(setter_two) {}

TwoIntObjectMutator::~TwoIntObjectMutator() {}

void TwoIntObjectMutator::SetToEnd(RLMachine& machine, GraphicsObject& object) {
  std::invoke(setter_one_, object.Param(), endval_one_);
  std::invoke(setter_two_, object.Param(), endval_two_);
}

std::unique_ptr<ObjectMutator> TwoIntObjectMutator::Clone() const {
  return std::make_unique<TwoIntObjectMutator>(*this);
}

void TwoIntObjectMutator::PerformSetting(RLMachine& machine,
                                         GraphicsObject& object) {
  int value = GetValueForTime(machine, startval_one_, endval_one_);
  std::invoke(setter_one_, object.Param(), value);

  value = GetValueForTime(machine, startval_two_, endval_two_);
  std::invoke(setter_two_, object.Param(), value);
}

// -----------------------------------------------------------------------

AdjustMutator::AdjustMutator(RLMachine& machine,
                             int repno,
                             int creation_time,
                             int duration_time,
                             int delay,
                             int type,
                             int start_x,
                             int target_x,
                             int start_y,
                             int target_y)
    : ObjectMutator(repno,
                    "objEveAdjust",
                    creation_time,
                    duration_time,
                    delay,
                    type),
      repno_(repno),
      start_x_(start_x),
      end_x_(target_x),
      start_y_(start_y),
      end_y_(target_y) {}

void AdjustMutator::SetToEnd(RLMachine& machine, GraphicsObject& object) {
  object.Param().SetXAdjustment(repno_, end_x_);
  object.Param().SetYAdjustment(repno_, end_y_);
}

std::unique_ptr<ObjectMutator> AdjustMutator::Clone() const {
  return std::make_unique<AdjustMutator>(*this);
}

void AdjustMutator::PerformSetting(RLMachine& machine, GraphicsObject& object) {
  int x = GetValueForTime(machine, start_x_, end_x_);
  object.Param().SetXAdjustment(repno_, x);

  int y = GetValueForTime(machine, start_y_, end_y_);
  object.Param().SetYAdjustment(repno_, y);
}

// -----------------------------------------------------------------------

DisplayMutator::DisplayMutator(RLMachine& machine,
                               GraphicsObject& object,
                               int creation_time,
                               int duration_time,
                               int delay,
                               int display,
                               int dip_event_mod,  // ignored
                               int tr_mod,
                               int move_mod,
                               int move_len_x,
                               int move_len_y,
                               int rotate_mod,
                               int rotate_count,
                               int scale_x_mod,
                               int scale_x_percent,
                               int scale_y_mod,
                               int scale_y_percent,
                               int sin_mod,
                               int sin_len,
                               int sin_count)
    : ObjectMutator(-1,
                    "objEveDisplay",
                    creation_time,
                    duration_time,
                    delay,
                    0),
      display_(display),
      tr_mod_(tr_mod),
      tr_start_(0),
      tr_end_(0),
      move_mod_(move_mod),
      move_start_x_(0),
      move_end_x_(0),
      move_start_y_(0),
      move_end_y_(0),
      rotate_mod_(rotate_mod),
      scale_x_mod_(scale_x_mod),
      scale_y_mod_(scale_y_mod) {
  if (tr_mod_) {
    tr_start_ = display ? 0 : 255;
    tr_end_ = display ? 255 : 0;
  }

  if (move_mod_) {
    if (display) {
      move_start_x_ = object.x() - move_len_x;
      move_end_x_ = object.x();
      move_start_y_ = object.y() - move_len_y;
      move_end_y_ = object.y();
    } else {
      move_start_x_ = object.x();
      move_end_x_ = object.x() + move_len_x;
      move_start_y_ = object.y();
      move_end_y_ = object.y() + move_len_y;
    }
  }

  if (rotate_mod_) {
    static bool printed = false;
    if (!printed) {
      std::cerr << "We don't support rotate mod yet." << std::endl;
      printed = true;
    }
  }

  if (scale_x_mod_) {
    static bool printed = false;
    if (!printed) {
      std::cerr << "We don't support scale X mod yet." << std::endl;
      printed = true;
    }
  }

  if (scale_y_mod_) {
    static bool printed = false;
    if (!printed) {
      std::cerr << "We don't support scale Y mod yet." << std::endl;
      printed = true;
    }
  }

  if (sin_mod) {
    static bool printed = false;
    if (!printed) {
      std::cerr << "  We don't support \"sin\" yet." << std::endl;
      printed = true;
    }
  }
}

void DisplayMutator::SetToEnd(RLMachine& machine, GraphicsObject& object) {
  auto& param = object.Param();

  param.SetVisible(display_);

  if (tr_mod_)
    param.SetAlpha(tr_end_);

  if (move_mod_) {
    param.SetX(move_end_x_);
    param.SetY(move_end_y_);
  }
}

std::unique_ptr<ObjectMutator> DisplayMutator::Clone() const {
  return std::make_unique<DisplayMutator>(*this);
}

void DisplayMutator::PerformSetting(RLMachine& machine,
                                    GraphicsObject& object) {
  auto& param = object.Param();
  param.SetVisible(true);

  if (tr_mod_) {
    int alpha = GetValueForTime(machine, tr_start_, tr_end_);
    param.SetAlpha(alpha);
  }

  if (move_mod_) {
    int x = GetValueForTime(machine, move_start_x_, move_end_x_);
    param.SetX(x);

    int y = GetValueForTime(machine, move_start_y_, move_end_y_);
    param.SetY(y);
  }
}
