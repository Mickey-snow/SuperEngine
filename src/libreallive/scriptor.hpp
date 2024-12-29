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

#include <memory>

namespace libreallive {

class Archive;
class Scenario;
class BytecodeElement;

struct ScriptLocation {
  ScriptLocation();
  ScriptLocation(int id, std::size_t offset);

  int scenario_id_;
  std::size_t offset_;
};

class Scriptor {
 public:
  explicit Scriptor(Archive& ar);
  ~Scriptor();

  using Reference = ScriptLocation;

  Reference Load(int scenario_number, unsigned long loc);
  Reference Load(int scenario_number);
  Reference LoadEntry(int scenario_number, int entry);

  unsigned long LocationNumber(Reference it) const;
  bool HasNext(Reference it) const;
  Reference Next(Reference it) const;
  const Scenario* GetScenario(Reference it) const;
  const std::shared_ptr<BytecodeElement>& Dereference(Reference it) const;

 private:
  Archive& archive_;
  mutable LRUCache<int, Scenario*> cached_scenario;
};

}  // namespace libreallive
