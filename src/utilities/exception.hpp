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

#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "libreallive/expression.hpp"

class RLMachine;
class RLOperation;

namespace libreallive {
class CommandElement;
}

using libreallive::ExpressionPiecesVector;

namespace rlvm {

class Exception : public std::exception {
 public:
  explicit Exception(const std::string& what);
  virtual ~Exception() throw();
  virtual const char* what() const throw();

  void setOperation(std::shared_ptr<RLOperation> operation) {
    operation_ = operation;
  }
  std::shared_ptr<RLOperation> operation() const { return operation_; }

 protected:
  // Returned in what().
  std::string description_;

  // Optionally, the operation that we were in when we threw.
  std::shared_ptr<RLOperation> operation_;
};

// An exception that rlvm can't internally recover from which should trigger a
// message alert box.
class UserPresentableError : public Exception {
 public:
  explicit UserPresentableError(const std::string& message_text,
                                const std::string& informative_text);
  virtual ~UserPresentableError() throw();

  const std::string& message_text() { return message_text_; }
  const std::string& informative_text() { return informative_text_; }

 protected:
  // The two lines in the alert dialog.
  std::string message_text_;
  std::string informative_text_;
};

class UnimplementedOpcode : public std::exception {
 public:
  UnimplementedOpcode(const std::string& funName,
                      int modtype,
                      int module,
                      int opcode,
                      int overload);
  UnimplementedOpcode(const std::string& name,
                      const libreallive::CommandElement& command);
  virtual ~UnimplementedOpcode() throw();

  std::string FormatCommand() const;
  std::string FormatParameters() const;

 private:
  std::string name;
  int module_type;
  int module_id;
  int opcode;
  int overload;

  ExpressionPiecesVector parameters;
};

}  // namespace rlvm
