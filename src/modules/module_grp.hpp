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

#pragma once

#include <deque>
#include <string>
#include <vector>

#include "machine/mapped_rlmodule.hpp"
#include "machine/rloperation.hpp"

// Contains functions for mod<1:33>, Grp.
class GrpModule : public MappedRLModule {
 public:
  GrpModule();
};

// An adapter for MappedRLModules that records the incoming command into the
// graphics stack.
std::shared_ptr<RLOperation> GraphicsStackMappingFun(
    std::unique_ptr<RLOperation> op);

// -----------------------------------------------------------------------

// Replays the new Graphics stack, string representations of reallive bytecode.
void ReplayGraphicsStackCommand(RLMachine& machine,
                                const std::deque<std::string>& stack);

// grp file last used. specified when filename is "???"
extern std::string default_grp_name;
