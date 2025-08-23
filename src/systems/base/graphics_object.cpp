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

#include "graphics_object.hpp"

#include "object/animator.hpp"
#include "object/objdrawer.hpp"
#include "object/object_mutator.hpp"
#include "utilities/exception.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------
// GraphicsObject
// -----------------------------------------------------------------------
GraphicsObject::GraphicsObject() = default;
GraphicsObject::~GraphicsObject() = default;

GraphicsObject GraphicsObject::Clone() const {
  GraphicsObject result;
  result.param_ = param_;
  if (object_data_) {
    result.object_data_ = object_data_->Clone();
  }

  result.object_mutators_.reserve(object_mutators_.size());
  for (const auto& it : object_mutators_)
    result.object_mutators_.emplace_back(it.DeepCopy());

  return result;
}

GraphicsObject::GraphicsObject(GraphicsObject&& rhs)
    : param_(rhs.param_), object_data_(nullptr), object_mutators_() {
  if (rhs.object_data_) {
    object_data_ = std::move(rhs.object_data_);
  }
  object_mutators_ = std::move(rhs.object_mutators_);

  rhs.param_ = ParameterManager();
}

GraphicsObject& GraphicsObject::operator=(GraphicsObject&& rhs) {
  param_ = rhs.param_;
  if (rhs.object_data_) {
    object_data_ = std::move(rhs.object_data_);
  } else {
    object_data_ = nullptr;
  }

  object_mutators_ = std::move(rhs.object_mutators_);

  rhs.param_ = ParameterManager();

  return *this;
}

void GraphicsObject::SetObjectData(GraphicsObjectData* obj) {
  object_data_.reset(obj);
}

void GraphicsObject::SetObjectData(std::unique_ptr<GraphicsObjectData> obj) {
  object_data_ = std::move(obj);
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
GraphicsObjectData& GraphicsObject::GetObjectData() {
  if (object_data_) {
    return *object_data_;
  } else {
    throw rlvm::Exception("null object data");
  }
}
void GraphicsObject::AddObjectMutator(ObjectMutator mutator) {
  // If there's a currently running mutator that matches the incoming mutator,
  // we ignore the incoming mutator. Kud Wafter's ED relies on this behavior.
  if (!IsMutatorRunningMatching(mutator.repr(), mutator.name()))
    object_mutators_.emplace_back(std::move(mutator));
}

bool GraphicsObject::IsMutatorRunningMatching(int repno,
                                              const std::string& name) {
  return std::any_of(
      object_mutators_.cbegin(), object_mutators_.cend(),
      [&](const auto& it) { return it.OperationMatches(repno, name); });
}

void GraphicsObject::EndObjectMutatorMatching(RLMachine& machine,
                                              int repno,
                                              const std::string& name,
                                              int speedup) {
  if (speedup == 0) {
    auto it = std::remove_if(object_mutators_.begin(), object_mutators_.end(),
                             [&](auto& it) {
                               if (!it.OperationMatches(repno, name))
                                 return false;
                               it.SetToEnd(this->Param());
                               return true;
                             });
    object_mutators_.erase(it, object_mutators_.end());

  } else if (speedup == 1) {
    // This is explicitly a noop.
  } else {
    std::cerr << "Warning: We only do immediate endings in "
              << "EndObjectMutatorMatching(). Unsupported speedup " << speedup
              << std::endl;
  }
}

void GraphicsObject::Render(int objNum, const GraphicsObject* parent) {
  if (object_data_ && Param().visible()) {
    object_data_->Render(*this, parent);
  }
}

RenderingConfig GraphicsObject::CreateRenderingConfig() const {
  RenderingConfig config;

  const auto& param = Param();
  config.blend_type = param.composite_mode();
  config.color = param.colour();
  config.tint = param.tint();
  config.mono = param.mono();
  config.invert = param.invert();
  config.light = param.light();

  return config;
}

void GraphicsObject::FreeObjectData() {
  object_data_.reset();
  object_mutators_.clear();
}

void GraphicsObject::InitializeParams() {
  param_ = ParameterManager();
  object_mutators_.clear();
}

void GraphicsObject::FreeDataAndInitializeParams() {
  object_data_.reset();
  param_ = ParameterManager();
  object_mutators_.clear();
}

void GraphicsObject::Execute(RLMachine& machine) {
  if (object_data_) {
    object_data_->Execute(machine);

    auto should_delete = [](GraphicsObjectData* it) -> bool {
      auto animator = it->GetAnimator();
      if (animator == nullptr)
        return false;
      return it->GetAnimator()->IsFinished() &&
             it->GetAnimator()->GetAfterAction() == AFTER_CLEAR;
    };
    if (should_delete(object_data_.get()))
      object_data_ = nullptr;
  }
}

void GraphicsObject::ExecuteMutators() {
  auto it = std::remove_if(object_mutators_.begin(), object_mutators_.end(),
                           [&](auto& it) { return it.Update(this->Param()); });
  object_mutators_.erase(it, object_mutators_.end());
}
