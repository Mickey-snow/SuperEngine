// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#include "machine/rlenvironment.hpp"

#include "core/gameexe.hpp"
#include "log/domain_logger.hpp"
#include "utilities/clock.hpp"

Generic& RLEnvironment::GetGenerics() { return generic_; }

void RLEnvironment::InitFrom(Gameexe& gexe) {
  generic_.val1 = gexe("INIT_ORIGINALSETING1_MOD").ToInt(0);
  generic_.val2 = gexe("INIT_ORIGINALSETING2_MOD").ToInt(0);
}

Stopwatch& RLEnvironment::GetTimer(int layer, int idx) {
  static DomainLogger logger("RLTimer");
  if (layer < 0 || layer >= 2 || idx < 0 || idx >= 255) {
    auto rec = logger(Severity::Warn);
    rec << "Invalid key provided when requesting timer. ";
    rec << "(layer=" << layer << " ,idx=" << idx << ')';
  }

  const auto key = std::make_pair(layer, idx);
  static std::shared_ptr<Clock> clock = std::make_shared<Clock>();
  if (!rltimer_.contains(key)) {
    Stopwatch timer(clock);
    timer.Apply(Stopwatch::Action::Run);
    rltimer_.emplace(key, std::move(timer));
  }
  return rltimer_.find(key)->second;
}
