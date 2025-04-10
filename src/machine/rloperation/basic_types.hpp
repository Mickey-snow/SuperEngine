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

#include "libreallive/bytecode_fwd.hpp"
#include "libreallive/expression.hpp"

#include <string>
#include <vector>

class RLMachine;
// Type definition for a Constant integer value.
//
// This struct is used to define the parameter types of a RLOperation
// subclass, and should not be used directly. It should only be used
// as a template parameter to one of those classes, or of another type
// definition struct.
struct IntConstant_T {
  // The output type of this type struct
  typedef int type;

  // Convert the incoming parameter objects into the resulting type
  static type getData(RLMachine& machine,
                      const libreallive::ExpressionPiecesVector& p,
                      unsigned int& position);

  enum { is_complex = false };
};

// Type definition for a constant string value.
//
// This struct is used to define the parameter types of a RLOperation
// subclass, and should not be used directly. It should only be used
// as a template parameter to one of those classes, or of another type
// definition struct.
struct StrConstant_T {
  // The output type of this type struct
  typedef std::string type;

  // Convert the incoming parameter objects into the resulting type
  static type getData(RLMachine& machine,
                      const libreallive::ExpressionPiecesVector& p,
                      unsigned int& position);

  enum { is_complex = false };
};

struct empty_struct {};

// Defines a null type for the Special parameter.
struct Empty_T {
  typedef empty_struct type;

  // Convert the incoming parameter objects into the resulting type.
  static type getData(RLMachine& machine,
                      const libreallive::ExpressionPiecesVector& p,
                      unsigned int& position) {
    return empty_struct();
  }

  enum { is_complex = false };
};
