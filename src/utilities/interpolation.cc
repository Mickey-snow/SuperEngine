// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "utilities/interpolation.h"

#include <cmath>
#include <stdexcept>

#include "utilities/exception.h"

double Interpolate(const InterpolationRange& range,
                   double amount,
                   InterpolationMode mode) {
  double cur = std::clamp(range.current, range.start, range.end);
  double percentage = (cur - range.start) / (range.end - range.start);
  static const double logBase = std::log(2.0);  // Cached log(2)

  switch (mode) {
    case InterpolationMode::Linear:
      return percentage * amount;

    case InterpolationMode::LogEaseOut: {
      // Eases out using logarithmic scaling
      double logPercentage = std::log(percentage + 1.0) / logBase;
      return logPercentage * amount;
    }

    case InterpolationMode::LogEaseIn: {
      // Eases in using inverse logarithmic scaling
      double logPercentage = std::log(percentage + 1.0) / logBase;
      return amount - (1.0 - logPercentage) * amount;
    }

    default:
      throw std::invalid_argument("Invalid interpolation mode: " +
                                  std::to_string(static_cast<int>(mode)));
  }
}

double InterpolateBetween(const InterpolationRange& range,
                          const Range& value,
                          InterpolationMode mode) {
  double to_add = value.end - value.start;
  return value.start + Interpolate(range, to_add, mode);
}
