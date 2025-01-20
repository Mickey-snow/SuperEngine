// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#pragma once

#include "machine/instruction.hpp"

#include <cstdint>

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

/**
 * @brief Interface for script loading and management operations.
 */
class IScriptor {
 public:
  virtual ~IScriptor() = default;

  virtual ScriptLocation Load(int scenario_number, unsigned long loc) = 0;
  virtual ScriptLocation Load(int scenario_number) = 0;
  virtual ScriptLocation LoadEntry(int scenario_number, int entry) = 0;

  virtual unsigned long LocationNumber(ScriptLocation it) const = 0;
  virtual bool HasNext(ScriptLocation it) const = 0;
  virtual ScriptLocation Next(ScriptLocation it) const = 0;
  virtual std::shared_ptr<Instruction> ResolveInstruction(
      ScriptLocation it) const = 0;

  virtual ScenarioConfig GetScenarioConfig(int scenario_number) const = 0;
};
