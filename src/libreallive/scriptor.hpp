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
#include "machine/iscriptor.hpp"

namespace libreallive {

class Archive;
class Scenario;
class BytecodeElement;

class Scriptor : public IScriptor {
 public:
  explicit Scriptor(Archive& ar);
  ~Scriptor() override;

  // IScriptor interface implementations
  ScriptLocation Load(int scenario_number, unsigned long loc) override;
  ScriptLocation Load(int scenario_number) override;
  ScriptLocation LoadEntry(int scenario_number, int entry) override;

  unsigned long LocationNumber(ScriptLocation it) const override;
  bool HasNext(ScriptLocation it) const override;
  ScriptLocation Next(ScriptLocation it) const override;
  Instruction ResolveInstruction(ScriptLocation it) const override;

  void SetDefaultScenarioConfig(ScenarioConfig cfg);
  ScenarioConfig GetScenarioConfig(int scenario_number) const override;

 private:
  Archive& archive_;
  ScenarioConfig default_config_;
  mutable LRUCache<int, Scenario*> cached_scenario;
};

}  // namespace libreallive
