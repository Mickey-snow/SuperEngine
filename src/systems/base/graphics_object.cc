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

// -----------------------------------------------------------------------
// GraphicsObject
// -----------------------------------------------------------------------
GraphicsObject::GraphicsObject() = default;
GraphicsObject::~GraphicsObject() = default;

GraphicsObject::GraphicsObject(const GraphicsObject& rhs) : param_(rhs.param_) {
  if (rhs.object_data_) {
    object_data_.reset(rhs.object_data_->Clone());
    object_data_->set_owned_by(*this);
  } else {
    object_data_.reset();
  }

  for (auto const& mutator : rhs.object_mutators_)
    object_mutators_.emplace_back(mutator->Clone());
}

GraphicsObject& GraphicsObject::operator=(const GraphicsObject& rhs) {
  DeleteObjectMutators();
  param_ = rhs.param_;

  if (rhs.object_data_) {
    object_data_.reset(rhs.object_data_->Clone());
    object_data_->set_owned_by(*this);
  } else {
    object_data_.reset();
  }

  for (auto const& mutator : rhs.object_mutators_)
    object_mutators_.emplace_back(mutator->Clone());

  return *this;
}

void GraphicsObject::SetObjectData(GraphicsObjectData* obj) {
  object_data_.reset(obj);
  object_data_->set_owned_by(*this);
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
void GraphicsObject::AddObjectMutator(std::unique_ptr<ObjectMutator> mutator) {
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
  param_ = ParameterManager();
  DeleteObjectMutators();
}

void GraphicsObject::FreeDataAndInitializeParams() {
  object_data_.reset();
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
