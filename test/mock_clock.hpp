// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utilities/clock.hpp"

class MockClock : public Clock {
 public:
  using duration_t = typename Clock::duration_t;
  using timepoint_t = typename Clock::timepoint_t;

  MockClock(timepoint_t epoch = std::chrono::steady_clock::now())
      : epoch_(epoch), elapsed_(0) {}

  virtual duration_t GetTicks() const override { return elapsed_; }
  virtual timepoint_t GetTime() const override { return epoch_ + elapsed_; }

  void SetEpoch(timepoint_t epoch) { epoch_ = epoch; }
  void SetElapsed(duration_t elapsed) { elapsed_ = elapsed; }
  void AdvanceTime(duration_t duration) { elapsed_ += duration; }

 private:
  timepoint_t epoch_;
  duration_t elapsed_;
};
