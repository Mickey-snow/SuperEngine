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

#pragma once

#include "core/frame_counter.hpp"
#include "object/parameter_manager.hpp"
#include "object/service_locator.hpp"
#include "systems/base/graphics_object.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>

class GraphicsObject;
class RLMachine;
class ParameterManager;

struct Mutator {
  using SetFn = std::function<void(ParameterManager&, int)>;
  SetFn setter_;
  std::shared_ptr<FrameCounter> fc_;
  bool Update(ParameterManager& pm) const {
    float value = fc_->ReadFrame();
    std::invoke(setter_, pm, static_cast<int>(value));
    return fc_->IsActive();
  }
};

class ObjectMutator {
  std::vector<Mutator> mutators_;
  int repr_;
  std::string name_;

 public:
  ObjectMutator(std::vector<Mutator> mut,
                int repr = 0,
                std::string name = "unknown")
      : mutators_(std::move(mut)), repr_(repr), name_(std::move(name)) {}

  inline void SetRepr(int in) { repr_ = in; }
  inline void SetName(std::string in) { name_.swap(in); }
  inline int repr() const { return repr_; }
  inline const std::string& name() const { return name_; }
  inline bool OperationMatches(int repr, const std::string& name) const {
    return repr_ == repr && name_ == name;
  }

  virtual bool operator()(RLMachine& machine, GraphicsObject& go) {
    RenderingService locator(machine);
    return this->operator()(locator, go.Param());
  }
  virtual bool operator()(RenderingService& locator, ParameterManager& pm) {
    locator.MarkObjStateDirty();
    return Update(pm);
  }

  bool Update(ParameterManager& pm) {
    auto it = std::remove_if(mutators_.begin(), mutators_.end(),
                             [&pm](const auto& it) { return it.Update(pm); });
    mutators_.erase(it, mutators_.end());
    return mutators_.empty();
  }

  void SetToEnd(ParameterManager& pm) {
    for (auto& it : mutators_) {
      it.fc_->EndTimer();
      it.Update(pm);
    }
  }
};
