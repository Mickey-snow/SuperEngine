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

#pragma once

enum class InterpolationMode { Linear = 0, LogEaseOut = 1, LogEaseIn = 2 };

struct Range {
  double start;
  double end;

  Range() : start(0.0), end(1.0) {}
  Range(double s, double e) : start(s), end(e) {}
};

struct InterpolationRange {
  double start;
  double current;
  double end;

  InterpolationRange() : start(0.0), current(0.0), end(1.0) {}
  InterpolationRange(double s, double c, double e)
      : start(s), current(c), end(e) {}
  InterpolationRange(int s, int c, int e)
      : start(static_cast<double>(s)),
        current(static_cast<double>(c)),
        end(static_cast<double>(e)) {}
};

// Interpolates between |start| and |end|. Returns a percentage of |amount|.
double Interpolate(const InterpolationRange& range,
                   double amount,
                   InterpolationMode mode);

// Interpolates a value between |value.start| and |value.end|.
double InterpolateBetween(const InterpolationRange& time,
                          const Range& value,
                          InterpolationMode mode);
