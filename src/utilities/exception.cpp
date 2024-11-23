// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#include "utilities/exception.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "libreallive/expression.hpp"
#include "libreallive/parser.hpp"

using std::ostringstream;

namespace rlvm {

// -----------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------

Exception::Exception(const std::string& what)
    : description_(what), operation_(NULL) {}

Exception::~Exception() throw() {}

const char* Exception::what() const throw() { return description_.c_str(); }

// -----------------------------------------------------------------------
// UserPresentableError
// -----------------------------------------------------------------------
UserPresentableError::UserPresentableError(const std::string& message_text,
                                           const std::string& informative_text)
    : Exception(message_text + ": " + informative_text),
      message_text_(message_text),
      informative_text_(informative_text) {}

UserPresentableError::~UserPresentableError() throw() {}

// -----------------------------------------------------------------------
// UnimplementedOpcode
// -----------------------------------------------------------------------
UnimplementedOpcode::UnimplementedOpcode(const std::string& funName,
                                         int modtype,
                                         int module,
                                         int opcode,
                                         int overload)
    : Exception("") {
  std::ostringstream oss;
  oss << funName << " (opcode<" << modtype << ":" << module << ":" << opcode
      << ", " << overload << ">)";
  name_ = oss.str();
  SetSimpleDescription();
}

UnimplementedOpcode::UnimplementedOpcode(
    RLMachine& machine,
    const std::string& funName,
    const libreallive::CommandElement& command)
    : Exception(""), parameters_(command.GetParsedParameters()) {
  std::ostringstream oss;
  oss << funName << " [opcode<" << command.modtype() << ":" << command.module()
      << ":" << command.opcode() << ", " << command.overload() << ">]";
  name_ = oss.str();
  SetFullDescription(machine);
}

UnimplementedOpcode::UnimplementedOpcode(
    RLMachine& machine,
    const libreallive::CommandElement& command)
    : Exception(""), parameters_(command.GetParsedParameters()) {
  ostringstream oss;
  oss << "opcode<" << command.modtype() << ":" << command.module() << ":"
      << command.opcode() << ", " << command.overload() << ">";
  name_ = oss.str();
  SetFullDescription(machine);
}

UnimplementedOpcode::~UnimplementedOpcode() throw() {}

void UnimplementedOpcode::SetFullDescription(RLMachine& machine) {
  ostringstream oss;
  oss << "Undefined: " << name_;

#ifndef NDEBUG
  const bool has_parameters_ = !parameters_.empty();
  if (has_parameters_) {
    bool first = true;
    oss << "(";
    for (const auto& it : parameters_) {
      if (!first) {
        oss << ", ";
      }
      first = false;
      oss << it->GetDebugString();
    }
    oss << ")";
  }
#endif

  description_ = oss.str();
}

void UnimplementedOpcode::SetSimpleDescription() {
  ostringstream oss;
  oss << "Undefined: " << name_;
  description_ = oss.str();
}

}  // namespace rlvm
