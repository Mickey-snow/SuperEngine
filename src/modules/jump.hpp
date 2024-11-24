// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

class RLMachine;

// Modifies the instruction pointer of the current stack frame to an entrypoint
void Jump(RLMachine&, int scenario, int entrypoint);

// Push a new frame with IP pointing to an entrypoint
void Farcall(RLMachine&, int scenario, int entrypoint);

// Moves the IP of the current stack frame to a location
void Goto(RLMachine&, unsigned long loc);

// Pushes a new frame with IP pointing to a given location
void Gosub(RLMachine&, unsigned long loc);

void Return(RLMachine&);

void ClearLongOperationsOffBackOfStack(RLMachine& machine);
