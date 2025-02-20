// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include "machine/mapped_rlmodule.hpp"

#include <string>

#include "machine/rloperation.hpp"

// -----------------------------------------------------------------------
// MappedRLModule
// -----------------------------------------------------------------------
MappedRLModule::MappedRLModule(const MappingFunction& fun,
                               const std::string& in_module_name,
                               int in_module_type,
                               int in_module_number)
    : RLModule(in_module_name, in_module_type, in_module_number),
      map_function_(fun) {}

MappedRLModule::~MappedRLModule() {}

void MappedRLModule::AddOpcode(int opcode,
                               unsigned char overload,
                               const std::string& name,
                               RLOperation* op) {
  RLModule::AddOpcode(opcode, overload, name, map_function_(op));
}
