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

// -----------------------------------------------------------------------
// struct ScriptLocation
// -----------------------------------------------------------------------

ScriptLocation::ScriptLocation() : scenario_id_(0), offset_(0) {}

ScriptLocation::ScriptLocation(int scenario_id, std::size_t off)
    : scenario_id_(scenario_id), offset_(off) {}

Scenario* FindScenario(Archive* archive, int scenario_number) {
  Scenario* sc = archive->GetScenario(scenario_number);
  if (!sc) {
    throw std::runtime_error("Scenario " + std::to_string(scenario_number) +
                             " not found.");
  }
  return sc;
}

// -----------------------------------------------------------------------
// Scriptor methods
// -----------------------------------------------------------------------

Scriptor::Scriptor(Archive& ar) : archive_(ar), cached_scenario(64) {}

Scriptor::~Scriptor() = default;

ScriptLocation Scriptor::Load(int scenario_number, unsigned long loc) {
  Scenario* sc = cached_scenario.fetch_or_else(
      scenario_number, std::bind(FindScenario, &archive_, scenario_number));

  const auto& elements = sc->script.elements_;
  auto comparator =
      [](const std::pair<unsigned long, std::shared_ptr<BytecodeElement>>& elem,
         unsigned long value) { return elem.first < value; };

  auto it =
      std::lower_bound(elements.cbegin(), elements.cend(), loc, comparator);
  if (it == elements.cend() || it->first != loc) {
    throw std::invalid_argument("Location " + std::to_string(loc) +
                                " not found in scenario " +
                                std::to_string(scenario_number) + ".");
  }

  return ScriptLocation(scenario_number, static_cast<std::size_t>(std::distance(
                                             elements.cbegin(), it)));
}

ScriptLocation Scriptor::Load(int scenario_number) {
  return ScriptLocation(scenario_number, 0);
}

ScriptLocation Scriptor::LoadEntry(int scenario_number, int entry) {
  Scenario* sc = cached_scenario.fetch_or_else(
      scenario_number, std::bind(FindScenario, &archive_, scenario_number));

  auto entry_it = sc->script.entrypoints_.find(entry);
  if (entry_it == sc->script.entrypoints_.end()) {
    throw std::invalid_argument("Entrypoint " + std::to_string(entry) +
                                " does not exist in scenario " +
                                std::to_string(scenario_number) + ".");
  }

  const unsigned long loc = entry_it->second;
  return Load(scenario_number, loc);
}

unsigned long Scriptor::LocationNumber(Scriptor::Reference it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_id_, std::bind(FindScenario, &archive_, it.scenario_id_));
  return sc->script.elements_[it.offset_].first;
}

bool Scriptor::HasNext(Scriptor::Reference it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_id_, std::bind(FindScenario, &archive_, it.scenario_id_));
  return it.offset_ < sc->script.elements_.size();
}

Scriptor::Reference Scriptor::Next(Scriptor::Reference it) const {
  ++it.offset_;
  return it;
}

const Scenario* Scriptor::GetScenario(Scriptor::Reference it) const {
  return cached_scenario.fetch_or_else(
      it.scenario_id_, std::bind(FindScenario, &archive_, it.scenario_id_));
}

const std::shared_ptr<BytecodeElement>& Scriptor::Dereference(
    Scriptor::Reference it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_id_, std::bind(FindScenario, &archive_, it.scenario_id_));
  return sc->script.elements_[it.offset_].second;
}

}  // namespace libreallive
