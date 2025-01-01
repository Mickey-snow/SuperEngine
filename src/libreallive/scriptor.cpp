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

// -----------------------------------------------------------------------
// struct ScriptLocation
// -----------------------------------------------------------------------

ScriptLocation::ScriptLocation() : scenario_number(0), location_offset(0) {}

ScriptLocation::ScriptLocation(int scenario_id, std::size_t off)
    : scenario_number(scenario_id), location_offset(off) {}

namespace libreallive {
// -----------------------------------------------------------------------
// class libreallive::Scriptor
// -----------------------------------------------------------------------

static Scenario* FindScenario(Archive* archive, int scenario_number) {
  Scenario* sc = archive->GetScenario(scenario_number);
  if (!sc) {
    throw std::runtime_error("Scenario " + std::to_string(scenario_number) +
                             " not found.");
  }
  return sc;
}

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

unsigned long Scriptor::LocationNumber(ScriptLocation it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_number,
      std::bind(FindScenario, &archive_, it.scenario_number));
  return sc->script.elements_[it.location_offset].first;
}

bool Scriptor::HasNext(ScriptLocation it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_number,
      std::bind(FindScenario, &archive_, it.scenario_number));
  return it.location_offset < sc->script.elements_.size();
}

ScriptLocation Scriptor::Next(ScriptLocation it) const {
  ++it.location_offset;
  return it;
}

std::shared_ptr<BytecodeElement> Scriptor::Dereference(
    ScriptLocation it) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      it.scenario_number,
      std::bind(FindScenario, &archive_, it.scenario_number));
  return sc->script.elements_[it.location_offset].second;
}

ScenarioConfig Scriptor::GetScenarioConfig(int scenario_number) const {
  const Scenario* sc = cached_scenario.fetch_or_else(
      scenario_number, std::bind(FindScenario, &archive_, scenario_number));

  ScenarioConfig cfg;
  cfg.text_encoding = sc->encoding();

  auto header = sc->header;
  auto value_or = [](int value, bool default_value) {
    switch (value) {
      case 1:
        return true;
      case 2:
        return false;
      default:
        return default_value;
    }
  };

  cfg.enable_message_savepoint = value_or(
      header.savepoint_message_, default_config_.enable_message_savepoint);
  cfg.enable_selcom_savepoint = value_or(
      header.savepoint_selcom_, default_config_.enable_selcom_savepoint);
  cfg.enable_seentop_savepoint = value_or(
      header.savepoint_seentop_, default_config_.enable_seentop_savepoint);

  return cfg;
}

void Scriptor::SetDefaultScenarioConfig(ScenarioConfig cfg) {
  default_config_ = std::move(cfg);
}

}  // namespace libreallive
