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

#include "libreallive/expression.hpp"
#include "libreallive/parser.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <string>

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
    : name(funName),
      module_type(modtype),
      module_id(module),
      opcode(opcode),
      overload(overload) {}

UnimplementedOpcode::UnimplementedOpcode(
    const std::string& name,
    const libreallive::CommandElement& command)
    : UnimplementedOpcode(name,
                          command.modtype(),
                          command.module(),
                          command.opcode(),
                          command.overload()) {
  parameters = command.GetParsedParameters();
}

UnimplementedOpcode::~UnimplementedOpcode() throw() {}

std::string UnimplementedOpcode::FormatCommand() const {
  std::string cmd_name = name.empty() ? "???" : name;
  std::string cmd_code =
      std::format("<{},{},{}:{}>", module_type, module_id, opcode, overload);
  return cmd_name + cmd_code;
}

std::string UnimplementedOpcode::FormatParameters() const {
  std::string result = "(";
  bool is_first = true;

  for (const auto& it : parameters) {
    if (!is_first)
      result += ',';
    is_first = false;
    result += it->GetDebugString();
  }

  result += ')';
  return result;
}

}  // namespace rlvm
