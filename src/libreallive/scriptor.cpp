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

#include "libreallive/scriptor.hpp"

#include "libreallive/archive.hpp"

#include <algorithm>
#include <stdexcept>

namespace libreallive {

Scriptor::Scriptor(Archive& ar) : archive_(ar) {}

Scriptor::~Scriptor() = default;

// -----------------------------------------------------------------------
// const_iterator
// -----------------------------------------------------------------------

Scriptor::const_iterator::const_iterator(const Scenario* sc, std::size_t off)
    : scenario_(sc), offset_(off) {}

int Scriptor::const_iterator::ScenarioNumber() const {
  return scenario_->scenario_number_;
}

uint32_t Scriptor::const_iterator::Location() const {
  return scenario_->script.elements_[offset_].first;
}

bool Scriptor::const_iterator::HasNext() const {
  return offset_ < scenario_->script.elements_.size();
}

const Scenario* Scriptor::const_iterator::GetScenario() const {
  return scenario_;
}

void Scriptor::const_iterator::increment() { ++offset_; }

bool Scriptor::const_iterator::equal(const const_iterator& other) const {
  return scenario_ == other.scenario_ && offset_ == other.offset_;
}

const std::shared_ptr<BytecodeElement>& Scriptor::const_iterator::dereference()
    const {
  return scenario_->script.elements_[offset_].second;
}

// -----------------------------------------------------------------------
// Scriptor methods
// -----------------------------------------------------------------------

Scriptor::const_iterator Scriptor::Load(int scenario_number, uint32_t loc) {
  Scenario* sc = archive_.GetScenario(scenario_number);
  if (!sc) {
    throw std::invalid_argument("Scenario " + std::to_string(scenario_number) +
                                " not found.");
  }

  const auto& elements = sc->script.elements_;
  auto comparator =
      [](const std::pair<uint32_t, std::shared_ptr<BytecodeElement>>& elem,
         uint32_t value) { return elem.first < value; };

  auto it =
      std::lower_bound(elements.cbegin(), elements.cend(), loc, comparator);
  if (it == elements.cend() || it->first != loc) {
    throw std::invalid_argument("Location " + std::to_string(loc) +
                                " not found in scenario " +
                                std::to_string(scenario_number) + ".");
  }

  return const_iterator(
      sc, static_cast<std::size_t>(std::distance(elements.cbegin(), it)));
}

Scriptor::const_iterator Scriptor::LoadEntry(int scenario_number, int entry) {
  Scenario* sc = archive_.GetScenario(scenario_number);
  if (!sc) {
    throw std::invalid_argument("Scenario " + std::to_string(scenario_number) +
                                " not found.");
  }

  auto entry_it = sc->script.entrypoints_.find(entry);
  if (entry_it == sc->script.entrypoints_.end()) {
    throw std::invalid_argument("Entrypoint " + std::to_string(entry) +
                                " does not exist in scenario " +
                                std::to_string(scenario_number) + ".");
  }

  uint32_t loc = entry_it->second;
  return Load(scenario_number, loc);
}

}  // namespace libreallive
