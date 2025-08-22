// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "object/object_mutator.hpp"

#include "core/frame_counter.hpp"
#include "object/parameter_manager.hpp"
#include "object/service_locator.hpp"
#include "systems/base/graphics_object.hpp"

#include <algorithm>
#include <functional>
#include <utility>

bool Mutator::Update(ParameterManager& pm) const {
  float value = fc_->ReadFrame();
  std::invoke(setter_, pm, static_cast<int>(value));
  return fc_->IsFinished();
}

ObjectMutator::ObjectMutator(std::vector<Mutator> mut,
                             int repr,
                             std::string name)
    : mutators_(std::move(mut)), repr_(repr), name_(std::move(name)) {}

ObjectMutator ObjectMutator::DeepCopy() const {
  std::vector<Mutator> mutators;
  mutators.reserve(mutators_.size());
  for (const auto& it : mutators_)
    mutators.emplace_back(Mutator{
        .setter_ = it.setter_,
        .fc_ = std::shared_ptr<FrameCounter>(it.fc_->Clone().release())});
  return ObjectMutator(std::move(mutators), repr_, name_);
}

void ObjectMutator::OnComplete(DoneFn fn) { on_complete_ = std::move(fn); }

bool ObjectMutator::operator()(RLMachine& machine, GraphicsObject& go) {
  RenderingService locator(machine);
  return this->operator()(locator, go.Param());
}

bool ObjectMutator::operator()(RenderingService& locator,
                               ParameterManager& pm) {
  locator.MarkObjStateDirty();
  return Update(pm);
}

bool ObjectMutator::Update(ParameterManager& pm) {
  auto it = std::remove_if(mutators_.begin(), mutators_.end(),
                           [&pm](const auto& it) { return it.Update(pm); });
  mutators_.erase(it, mutators_.end());
  const bool done = mutators_.empty();
  if (done && on_complete_)
    std::invoke(on_complete_, pm);
  return done;
}

void ObjectMutator::SetToEnd(ParameterManager& pm) {
  for (auto& it : mutators_) {
    it.fc_->EndTimer();
    it.Update(pm);
  }

  if (on_complete_)
    std::invoke(on_complete_, pm);
}
