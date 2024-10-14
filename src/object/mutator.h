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

#ifndef SRC_OBJECT_MUTATOR_H_
#define SRC_OBJECT_MUTATOR_H_

#include "utilities/interpolation.h"

#include "object/parameter_manager.h"
#include "object/service_locator.h"
#include "systems/base/graphics_object.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>

class GraphicsObject;
class RLMachine;
class ParameterManager;

// Pure abstract interface for object mutators.
class ObjectMutator {
 public:
  virtual ~ObjectMutator() = default;

  virtual int repr() const = 0;
  virtual const std::string& name() const = 0;

  // Called every tick. Returns true if the command has completed.
  virtual bool operator()(RLMachine&, GraphicsObject& object) = 0;
  virtual bool operator()(MutatorService&, GraphicsObject& object) = 0;

  // Returns true if this ObjectMutator is operating on |repr| and |name|.
  virtual bool OperationMatches(int repr, const std::string& name) const = 0;

  // Called to end the mutation prematurely.
  virtual void SetToEnd(GraphicsObject& object) = 0;

  // Builds a copy of the ObjectMutator. Used during object promotion.
  virtual std::unique_ptr<ObjectMutator> Clone() const = 0;
};

// -----------------------------------------------------------------------

// An object mutator that takes a single integer.
class OneIntObjectMutator : public ObjectMutator {
 public:
  using Setter = std::function<void(ParameterManager&, int)>;

  OneIntObjectMutator(const std::string& name,
                      int creation_time,
                      int duration_time,
                      int delay,
                      int type,
                      int start_value,
                      int target_value,
                      Setter setter)
      : repr_(-1),
        name_(name),
        creation_time_(creation_time),
        duration_time_(duration_time),
        delay_(delay),
        type_(static_cast<InterpolationMode>(type)),
        startval_(start_value),
        endval_(target_value),
        setter_(setter) {}
  virtual ~OneIntObjectMutator() {}

  virtual int repr() const override { return repr_; }
  virtual const std::string& name() const override { return name_; }

  virtual bool operator()(RLMachine& machine, GraphicsObject& object) override {
    MutatorService locator(machine);
    return this->operator()(locator, object);
  }
  virtual bool operator()(MutatorService& locator,
                          GraphicsObject& object) override {
    unsigned int ticks = locator.GetTicks();
    if (ticks > (creation_time_ + delay_)) {
      PerformSetting(locator, object);
      locator.MarkObjStateDirty();
    }
    return ticks > (creation_time_ + delay_ + duration_time_);
  }

  virtual bool OperationMatches(int repr,
                                const std::string& name) const override {
    return repr_ == repr && name_ == name;
  }

  virtual void SetToEnd(GraphicsObject& object) override {
    std::invoke(setter_, object.Param(), endval_);
  }

  virtual std::unique_ptr<ObjectMutator> Clone() const override {
    return std::make_unique<OneIntObjectMutator>(*this);
  }

 protected:
  int GetValueForTime(MutatorService& locator, int start, int end) {
    unsigned int ticks = locator.GetTicks();
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

  void PerformSetting(MutatorService& locator, GraphicsObject& object) {
    int value = GetValueForTime(locator, startval_, endval_);
    std::invoke(setter_, object.Param(), value);
  }

 private:
  int repr_;
  std::string name_;
  int creation_time_;
  int duration_time_;
  int delay_;
  InterpolationMode type_;

  int startval_;
  int endval_;
  Setter setter_;
};

// -----------------------------------------------------------------------

// An object mutator that takes a repno and an integer.
class RepnoIntObjectMutator : public ObjectMutator {
 public:
  using Setter = std::function<void(ParameterManager&, int, int)>;

  RepnoIntObjectMutator(const std::string& name,
                        int creation_time,
                        int duration_time,
                        int delay,
                        int type,
                        int repno,
                        int start_value,
                        int target_value,
                        Setter setter)
      : repr_(repno),
        name_(name),
        creation_time_(creation_time),
        duration_time_(duration_time),
        delay_(delay),
        type_(static_cast<InterpolationMode>(type)),
        repno_(repno),
        startval_(start_value),
        endval_(target_value),
        setter_(setter) {}
  virtual ~RepnoIntObjectMutator() {}

  virtual int repr() const override { return repr_; }
  virtual const std::string& name() const override { return name_; }

  virtual bool operator()(RLMachine& machine, GraphicsObject& object) override {
    MutatorService locator(machine);
    return this->operator()(locator, object);
  }
  virtual bool operator()(MutatorService& locator,
                          GraphicsObject& object) override {
    unsigned int ticks = locator.GetTicks();
    if (ticks > (creation_time_ + delay_)) {
      PerformSetting(locator, object);
      locator.MarkObjStateDirty();
    }
    return ticks > (creation_time_ + delay_ + duration_time_);
  }

  virtual bool OperationMatches(int repr,
                                const std::string& name) const override {
    return repr_ == repr && name_ == name;
  }

  virtual void SetToEnd(GraphicsObject& object) override {
    std::invoke(setter_, object.Param(), repno_, endval_);
  }

  virtual std::unique_ptr<ObjectMutator> Clone() const override {
    return std::make_unique<RepnoIntObjectMutator>(*this);
  }

 protected:
  int GetValueForTime(MutatorService& locator, int start, int end) {
    unsigned int ticks = locator.GetTicks();
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

  void PerformSetting(MutatorService& locator, GraphicsObject& object) {
    int value = GetValueForTime(locator, startval_, endval_);
    std::invoke(setter_, object.Param(), repno_, value);
  }

 private:
  int repr_;
  std::string name_;
  int creation_time_;
  int duration_time_;
  int delay_;
  InterpolationMode type_;

  int repno_;
  int startval_;
  int endval_;
  Setter setter_;
};

// -----------------------------------------------------------------------

// An object mutator that varies two integers.
class TwoIntObjectMutator : public ObjectMutator {
 public:
  using Setter = std::function<void(ParameterManager&, int)>;

  TwoIntObjectMutator(const std::string& name,
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
      : repr_(-1),
        name_(name),
        creation_time_(creation_time),
        duration_time_(duration_time),
        delay_(delay),
        type_(static_cast<InterpolationMode>(type)),
        startval_one_(start_one),
        endval_one_(target_one),
        setter_one_(setter_one),
        startval_two_(start_two),
        endval_two_(target_two),
        setter_two_(setter_two) {}
  virtual ~TwoIntObjectMutator() {}

  virtual int repr() const override { return repr_; }
  virtual const std::string& name() const override { return name_; }

  virtual bool operator()(RLMachine& machine, GraphicsObject& object) override {
    MutatorService locator(machine);
    return this->operator()(locator, object);
  }
  virtual bool operator()(MutatorService& locator,
                          GraphicsObject& object) override {
    unsigned int ticks = locator.GetTicks();
    if (ticks > (creation_time_ + delay_)) {
      PerformSetting(locator, object);
      locator.MarkObjStateDirty();
    }
    return ticks > (creation_time_ + delay_ + duration_time_);
  }

  virtual bool OperationMatches(int repr,
                                const std::string& name) const override {
    return repr_ == repr && name_ == name;
  }

  virtual void SetToEnd(GraphicsObject& object) override {
    std::invoke(setter_one_, object.Param(), endval_one_);
    std::invoke(setter_two_, object.Param(), endval_two_);
  }

  virtual std::unique_ptr<ObjectMutator> Clone() const override {
    return std::make_unique<TwoIntObjectMutator>(*this);
  }

 protected:
  int GetValueForTime(MutatorService& locator, int start, int end) {
    unsigned int ticks = locator.GetTicks();
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

  void PerformSetting(MutatorService& locator, GraphicsObject& object) {
    int value_one = GetValueForTime(locator, startval_one_, endval_one_);
    std::invoke(setter_one_, object.Param(), value_one);

    int value_two = GetValueForTime(locator, startval_two_, endval_two_);
    std::invoke(setter_two_, object.Param(), value_two);
  }

 private:
  int repr_;
  std::string name_;
  int creation_time_;
  int duration_time_;
  int delay_;
  InterpolationMode type_;

  int startval_one_;
  int endval_one_;
  Setter setter_one_;

  int startval_two_;
  int endval_two_;
  Setter setter_two_;
};

// -----------------------------------------------------------------------

class AdjustMutator : public ObjectMutator {
 public:
  AdjustMutator(RLMachine& machine,
                int repno,
                int creation_time,
                int duration_time,
                int delay,
                int type,
                int start_x,
                int target_x,
                int start_y,
                int target_y)
      : repr_(repno),
        name_("objEveAdjust"),
        creation_time_(creation_time),
        duration_time_(duration_time),
        delay_(delay),
        type_(static_cast<InterpolationMode>(type)),
        repno_(repno),
        start_x_(start_x),
        end_x_(target_x),
        start_y_(start_y),
        end_y_(target_y) {}

  virtual ~AdjustMutator() {}

  virtual int repr() const override { return repr_; }
  virtual const std::string& name() const override { return name_; }

  virtual bool operator()(RLMachine& machine, GraphicsObject& object) override {
    MutatorService locator(machine);
    return this->operator()(locator, object);
  }
  virtual bool operator()(MutatorService& locator,
                          GraphicsObject& object) override {
    unsigned int ticks = locator.GetTicks();
    if (ticks > (creation_time_ + delay_)) {
      PerformSetting(locator, object);
      locator.MarkObjStateDirty();
    }
    return ticks > (creation_time_ + delay_ + duration_time_);
  }

  virtual bool OperationMatches(int repr,
                                const std::string& name) const override {
    return repr_ == repr && name_ == name;
  }

  virtual void SetToEnd(GraphicsObject& object) override {
    object.Param().SetXAdjustment(repno_, end_x_);
    object.Param().SetYAdjustment(repno_, end_y_);
  }

  virtual std::unique_ptr<ObjectMutator> Clone() const override {
    return std::make_unique<AdjustMutator>(*this);
  }

 protected:
  int GetValueForTime(MutatorService& machine, int start, int end) {
    unsigned int ticks = machine.GetTicks();
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

  void PerformSetting(MutatorService& machine, GraphicsObject& object) {
    int x = GetValueForTime(machine, start_x_, end_x_);
    object.Param().SetXAdjustment(repno_, x);

    int y = GetValueForTime(machine, start_y_, end_y_);
    object.Param().SetYAdjustment(repno_, y);
  }

 private:
  int repr_;
  std::string name_;
  int creation_time_;
  int duration_time_;
  int delay_;
  InterpolationMode type_;

  int repno_;
  int start_x_;
  int end_x_;
  int start_y_;
  int end_y_;
};

// -----------------------------------------------------------------------

class DisplayMutator : public ObjectMutator {
 public:
  DisplayMutator(RLMachine& machine,
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
      : repr_(-1),
        name_("objEveDisplay"),
        creation_time_(creation_time),
        duration_time_(duration_time),
        delay_(delay),
        type_(InterpolationMode::Linear),
        display_(display),
        tr_mod_(tr_mod),
        move_mod_(move_mod),
        rotate_mod_(rotate_mod),
        scale_x_mod_(scale_x_mod),
        scale_y_mod_(scale_y_mod),
        tr_start_(0),
        tr_end_(0),
        move_start_x_(0),
        move_end_x_(0),
        move_start_y_(0),
        move_end_y_(0) {
    if (tr_mod_) {
      tr_start_ = display ? 0 : 255;
      tr_end_ = display ? 255 : 0;
    }

    if (move_mod_) {
      auto& param = object.Param();
      if (display) {
        move_start_x_ = param.x() - move_len_x;
        move_end_x_ = param.x();
        move_start_y_ = param.y() - move_len_y;
        move_end_y_ = param.y();
      } else {
        move_start_x_ = param.x();
        move_end_x_ = param.x() + move_len_x;
        move_start_y_ = param.y();
        move_end_y_ = param.y() + move_len_y;
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

  virtual ~DisplayMutator() {}

  virtual int repr() const override { return repr_; }
  virtual const std::string& name() const override { return name_; }

  virtual bool operator()(RLMachine& machine, GraphicsObject& object) override {
    MutatorService locator(machine);
    return this->operator()(locator, object);
  }
  virtual bool operator()(MutatorService& locator,
                          GraphicsObject& object) override {
    unsigned int ticks = locator.GetTicks();
    if (ticks > (creation_time_ + delay_)) {
      PerformSetting(locator, object);
      locator.MarkObjStateDirty();
    }
    return ticks > (creation_time_ + delay_ + duration_time_);
  }

  virtual bool OperationMatches(int repr,
                                const std::string& name) const override {
    return repr_ == repr && name_ == name;
  }

  virtual void SetToEnd(GraphicsObject& object) override {
    auto& param = object.Param();

    param.SetVisible(display_);

    if (tr_mod_)
      param.SetAlpha(tr_end_);

    if (move_mod_) {
      param.SetX(move_end_x_);
      param.SetY(move_end_y_);
    }
  }

  virtual std::unique_ptr<ObjectMutator> Clone() const override {
    return std::make_unique<DisplayMutator>(*this);
  }

 protected:
  int GetValueForTime(MutatorService& locator, int start, int end) {
    unsigned int ticks = locator.GetTicks();
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

  void PerformSetting(MutatorService& locator, GraphicsObject& object) {
    auto& param = object.Param();
    param.SetVisible(true);

    if (tr_mod_) {
      int alpha = GetValueForTime(locator, tr_start_, tr_end_);
      param.SetAlpha(alpha);
    }

    if (move_mod_) {
      int x = GetValueForTime(locator, move_start_x_, move_end_x_);
      param.SetX(x);

      int y = GetValueForTime(locator, move_start_y_, move_end_y_);
      param.SetY(y);
    }
  }

 private:
  int repr_;
  std::string name_;
  int creation_time_;
  int duration_time_;
  int delay_;
  InterpolationMode type_;

  bool display_;
  bool tr_mod_;
  bool move_mod_;
  bool rotate_mod_;
  bool scale_x_mod_;
  bool scale_y_mod_;

  int tr_start_;
  int tr_end_;

  int move_start_x_;
  int move_end_x_;
  int move_start_y_;
  int move_end_y_;
};

#endif
