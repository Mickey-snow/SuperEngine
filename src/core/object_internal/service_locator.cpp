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
// -----------------------------------------------------------------------

#include "service_locator.hpp"

#include "machine/rlmachine.hpp"
#include "core/object_internal/drawer/parent.hpp"
#include "core/object_internal/objdrawer.hpp"
#include "core/object.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "systems/event_system.hpp"

RenderingService::RenderingService(RLMachine& imachine)
    : system_(imachine.GetSystem()) {}

RenderingService::RenderingService(System& isystem) : system_(isystem) {}

unsigned int RenderingService::GetTicks() const {
  return system_.event().GetTicks();
}

void RenderingService::MarkObjStateDirty() const {}

void RenderingService::MarkScreenDirty(GraphicsUpdateType type) {}
