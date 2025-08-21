// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#include "machine/rlmodule.hpp"

#include "libreallive/parser.hpp"
#include "machine/general_operations.hpp"
#include "machine/rloperation.hpp"
#include "utilities/exception.hpp"

#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------
// RLMoudle
// -----------------------------------------------------------------------

RLModule::RLModule(const std::string& in_module_name,
                   int in_module_type,
                   int in_module_number)
    : property_list_(),
      module_type_(in_module_type),
      module_number_(in_module_number),
      module_name_(in_module_name) {}

RLModule::~RLModule() {}

void RLModule::AddOpcode(int opcode,
                         unsigned char overload,
                         std::string name,
                         std::shared_ptr<RLOperation> op) {
  op->SetName(name);

  auto emplace_result = stored_operations_.try_emplace(
      std::make_pair(opcode, static_cast<int>(overload)), op);
  if (!emplace_result.second) {
    throw std::invalid_argument(
        std::format("Duplicate opcode in {}: opcode {},{}", GetDebugString(),
                    opcode, overload));
  }
}
void RLModule::AddOpcode(int opcode,
                         unsigned char overload,
                         char const* name,
                         RLOperation* op) {
  AddOpcode(opcode, overload, std::string(name),
            std::shared_ptr<RLOperation>(op));
}

void RLModule::AddUnsupportedOpcode(int opcode,
                                    unsigned char overload,
                                    char const* name) {
  AddOpcode(opcode, overload, name,
            std::make_shared<UndefinedFunction>(module_type_, module_number_,
                                                opcode, (int)overload));
}

void RLModule::SetProperty(int property, int value) {
  std::map<std::pair<int, int>, std::shared_ptr<RLOperation>> const&
      operations = GetStoredOperations();
  for (auto& it :
       const_cast<std::map<std::pair<int, int>, std::shared_ptr<RLOperation>>&>(
           operations)) {
    it.second->SetProperty(property, value);
  }
}

bool RLModule::GetProperty(int property, int& value) const {
  if (property_list_) {
    PropertyList::iterator it = FindProperty(property);
    if (it != property_list_->end()) {
      value = it->second;
      return true;
    }
  }

  return false;
}

RLModule::PropertyList::iterator RLModule::FindProperty(int property) const {
  return find_if(property_list_->begin(), property_list_->end(),
                 [&](Property& p) { return p.first == property; });
}

std::map<std::pair<int, int>, std::shared_ptr<RLOperation>> const&
RLModule::GetStoredOperations() const {
  return stored_operations_;
}

std::string RLModule::GetDebugString() const {
  return std::format("mod<{},{}:{}>", module_name(), module_type(),
                     module_number());
}

std::ostream& operator<<(std::ostream& os, const RLModule& mod) {
  return os << mod.GetDebugString();
}
