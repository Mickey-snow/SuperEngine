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

#include "machine/rloperation.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "libreallive/parser.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation/reference_types.hpp"
#include "utilities/exception.hpp"

// -----------------------------------------------------------------------
// RLOperation
// -----------------------------------------------------------------------

RLOperation::RLOperation() : name_() {}

RLOperation::~RLOperation() {}

RLOperation* RLOperation::SetProperty(int property, int value) {
  if (!property_list_) {
    property_list_.reset(new std::vector<std::pair<int, int>>);
  }

  // Modify the property if it already exists
  PropertyList::iterator it = FindProperty(property);
  if (it != property_list_->end()) {
    it->second = value;
  } else {
    property_list_->emplace_back(property, value);
  }

  return this;
}

bool RLOperation::GetProperty(int property, int& value) const {
  if (property_list_) {
    PropertyList::iterator it = FindProperty(property);
    if (it != property_list_->end()) {
      value = it->second;
      return true;
    }
  }

  if (module_) {
    // If we don't have a property, ask our module if it has one.
    return module_->GetProperty(property, value);
  }

  return false;
}

RLOperation::PropertyList::iterator RLOperation::FindProperty(
    int property) const {
  return find_if(property_list_->begin(), property_list_->end(),
                 [&](Property& p) { return p.first == property; });
}

bool RLOperation::ShouldAdvanceIP() { return true; }

void RLOperation::DispatchFunction(RLMachine& machine,
                                   const libreallive::CommandElement& ff) {
  const libreallive::ExpressionPiecesVector& parameter_pieces =
      ff.GetParsedParameters();

  // Now Dispatch based on these parameters.
  Dispatch(machine, parameter_pieces);

  // By default, we advacne the instruction pointer on any instruction we
  // perform. Weird special cases all derive from RLOp_SpecialCase, which
  // redefines the Dispatcher, so this is ok.
  if (ShouldAdvanceIP())
    machine.AdvanceInstructionPointer();
}

void RLOp_SpecialCase::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& parameters) {
  throw rlvm::Exception("Tried to call empty RLOp_SpecialCase::Dispatch().");
}

void RLOp_SpecialCase::ParseParameters(
    const std::vector<std::string>& input,
    libreallive::ExpressionPiecesVector& output) {
  for (auto const& parameter : input) {
    const char* src = parameter.c_str();
    output.push_back(libreallive::ExpressionParser::GetData(src));
  }
}

void RLOp_SpecialCase::DispatchFunction(RLMachine& machine,
                                        const libreallive::CommandElement& ff) {
  // Pass this on to the implementation of this functor.
  operator()(machine, ff);
}

template <>
void RLNormalOpcode<>::ParseParameters(
    const std::vector<std::string>& input,
    libreallive::ExpressionPiecesVector& output) {}

template <>
void RLOpcode<>::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& parameters) {
  operator()(machine);
}

// Default instantiations.
template class RLNormalOpcode<>;
template class RLNormalOpcode<IntConstant_T>;
template class RLNormalOpcode<IntConstant_T, IntConstant_T>;
template class RLNormalOpcode<IntConstant_T, StrConstant_T>;
template class RLNormalOpcode<IntConstant_T, IntConstant_T, IntConstant_T>;
template class RLNormalOpcode<IntConstant_T,
                              IntConstant_T,
                              IntConstant_T,
                              IntConstant_T>;
template class RLNormalOpcode<IntReference_T>;
template class RLNormalOpcode<IntReference_T, IntReference_T>;
template class RLNormalOpcode<StrConstant_T>;
template class RLNormalOpcode<StrConstant_T, IntConstant_T>;
template class RLNormalOpcode<StrConstant_T, StrConstant_T>;
template class RLNormalOpcode<StrReference_T>;

template class RLOpcode<>;
template class RLOpcode<IntConstant_T>;
template class RLOpcode<IntConstant_T, IntConstant_T>;
template class RLOpcode<IntConstant_T, StrConstant_T>;
template class RLOpcode<IntConstant_T, IntConstant_T, IntConstant_T>;
template class RLOpcode<IntConstant_T,
                        IntConstant_T,
                        IntConstant_T,
                        IntConstant_T>;
template class RLOpcode<IntReference_T>;
template class RLOpcode<IntReference_T, IntReference_T>;
template class RLOpcode<StrConstant_T>;
template class RLOpcode<StrConstant_T, IntConstant_T>;
template class RLOpcode<StrConstant_T, StrConstant_T>;
template class RLOpcode<StrReference_T>;
