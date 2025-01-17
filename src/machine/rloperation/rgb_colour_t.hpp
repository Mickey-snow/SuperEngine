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

#include <string>
#include <vector>

#include "core/colour.hpp"
#include "machine/rloperation.hpp"

struct RGBColour_T {
  typedef RGBAColour type;

  static type getData(RLMachine& machine,
                      const libreallive::ExpressionPiecesVector& p,
                      unsigned int& position) {
    int r = IntConstant_T::getData(machine, p, position);
    int g = IntConstant_T::getData(machine, p, position);
    int b = IntConstant_T::getData(machine, p, position);
    return RGBAColour(r, g, b);
  }

  enum { is_complex = false };
};

// RGB colour triplet with option alpha.
struct RGBMaybeAColour_T {
  typedef RGBAColour type;

  static type getData(RLMachine& machine,
                      const libreallive::ExpressionPiecesVector& p,
                      unsigned int& position) {
    int r = IntConstant_T::getData(machine, p, position);
    int g = IntConstant_T::getData(machine, p, position);
    int b = IntConstant_T::getData(machine, p, position);

    int a;
    if (position < p.size()) {
      a = IntConstant_T::getData(machine, p, position);
    } else {
      a = 255;
    }

    return RGBAColour(r, g, b, a);
  }

  enum { is_complex = false };
};
