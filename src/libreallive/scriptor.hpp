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

#pragma once

#include "lru_cache.hpp"
#include "machine/instruction.hpp"

#include <memory>

struct ScriptLocation {
  ScriptLocation();
  ScriptLocation(int id, std::size_t offset);

  int scenario_number;
  std::size_t location_offset;

  void serialize(auto& ar, unsigned int version) {
    ar & scenario_number & location_offset;
  }
};

struct ScenarioConfig {
  int text_encoding;
  bool enable_message_savepoint;
  bool enable_selcom_savepoint;
  bool enable_seentop_savepoint;
};

namespace libreallive {

class Archive;
class Scenario;
class BytecodeElement;

class Scriptor {
 public:
  explicit Scriptor(Archive& ar);
  ~Scriptor();

  ScriptLocation Load(int scenario_number, unsigned long loc);
  ScriptLocation Load(int scenario_number);
  ScriptLocation LoadEntry(int scenario_number, int entry);

  unsigned long LocationNumber(ScriptLocation it) const;
  bool HasNext(ScriptLocation it) const;
  ScriptLocation Next(ScriptLocation it) const;
  Instruction ResolveInstruction(ScriptLocation it) const;

  void SetDefaultScenarioConfig(ScenarioConfig cfg);
  ScenarioConfig GetScenarioConfig(int scenario_number) const;

 private:
  Archive& archive_;
  ScenarioConfig default_config_;
  mutable LRUCache<int, Scenario*> cached_scenario;
};

}  // namespace libreallive
