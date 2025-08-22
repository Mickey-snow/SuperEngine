// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include <gtest/gtest.h>

#include "core/frame_counter.hpp"
#include "mock_clock.hpp"
#include "utilities/clock.hpp"

#include <chrono>
#include <memory>

using std::chrono_literals::operator""ms;

class FrameCounterTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockClock> clock_;

  void SetUp() override {
    clock_ = std::make_shared<MockClock>();
    clock_->SetElapsed(0ms);
  }
};

TEST_F(FrameCounterTest, LinearProgression) {
  SimpleFrameCounter counter(clock_, /*frame_min=*/0, /*frame_max=*/10,
                             /*milliseconds=*/1000);

  auto delay = 100ms;
  counter.BeginTimer(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0)
      << "At time=-100ms, the frame should be 0 (initial value)";

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0)
      << "At time=0, the frame should be 0 (initial value)";

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 1)
      << "after 100 ms, we expect the value to "
         "be about 1.0 => integer cast is 1";

  clock_->AdvanceTime(400ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5)
      << "total of 500 ms => half the time => ~5.0";

  clock_->AdvanceTime(500ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 10);

  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, LinearZeroDuration) {
  SimpleFrameCounter counter(clock_, /*frame_min=*/5, /*frame_max=*/10,
                             /*milliseconds=*/0);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 10);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, SimpleFrameCounter_MinEqualsMax) {
  SimpleFrameCounter counter(clock_, 5, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);
  clock_->AdvanceTime(500ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);
}

TEST_F(FrameCounterTest, SimpleFrameCounter_EarlyEnd) {
  SimpleFrameCounter counter(clock_, 0, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  EXPECT_TRUE(counter.IsActive());

  clock_->AdvanceTime(10ms);
  counter.EndTimer();
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, BasicLoop) {
  LoopFrameCounter counter(clock_, 0, 3, 300);

  auto delay = 100ms;
  counter.BeginTimer(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  clock_->AdvanceTime(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 1);
  clock_->AdvanceTime(200ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  EXPECT_TRUE(counter.IsActive())
      << "Because it's a loop, we do not end the timer; it resets to min.";

  clock_->AdvanceTime(10ms);

  int frame_val = counter.ReadFrame();
  EXPECT_GE(frame_val, 0);
  EXPECT_LE(frame_val, 1);
}

TEST_F(FrameCounterTest, BasicLoop_EarlyEnd) {
  LoopFrameCounter counter(clock_, 0, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  EXPECT_TRUE(counter.IsActive());

  clock_->AdvanceTime(100ms);
  counter.EndTimer();
  EXPECT_FLOAT_EQ(counter.ReadFrame(), .5);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, BasicTurn) {
  TurnFrameCounter counter(clock_, 2, 5, 300);

  auto delay = 100ms;
  counter.BeginTimer(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 2);

  clock_->AdvanceTime(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 2);

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 3);

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 4);

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);

  clock_->AdvanceTime(100ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 4);
}

TEST_F(FrameCounterTest, BasicTurn_EarlyEnd) {
  TurnFrameCounter counter(clock_, 0, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  EXPECT_TRUE(counter.IsActive());

  clock_->AdvanceTime(1100ms);
  counter.EndTimer();
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 4.5);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, AcceleratingFrameCounter) {
  AcceleratingFrameCounter counter(clock_, 0, 10, 1000);

  auto delay = 100ms;
  counter.BeginTimer(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  clock_->AdvanceTime(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  clock_->AdvanceTime(500ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 2.5);

  clock_->AdvanceTime(500ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 10);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, AcceleratingFrameCounter_EarlyEnd) {
  AcceleratingFrameCounter counter(clock_, 0, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  EXPECT_TRUE(counter.IsActive());

  clock_->AdvanceTime(10ms);
  counter.EndTimer();
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, DeceleratingFrameCounter) {
  DeceleratingFrameCounter counter(clock_, 0, 10, 1000);

  auto delay = 100ms;
  counter.BeginTimer(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  clock_->AdvanceTime(delay);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);

  clock_->AdvanceTime(100ms);
  EXPECT_GE(counter.ReadFrame(), 1);

  clock_->AdvanceTime(900ms);
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 10);
  EXPECT_FALSE(counter.IsActive());
}

TEST_F(FrameCounterTest, DeceleratingFrameCounter_EarlyEnd) {
  DeceleratingFrameCounter counter(clock_, 0, 5, 1000);

  EXPECT_FLOAT_EQ(counter.ReadFrame(), 0);
  EXPECT_TRUE(counter.IsActive());

  clock_->AdvanceTime(10ms);
  counter.EndTimer();
  EXPECT_FLOAT_EQ(counter.ReadFrame(), 5);
  EXPECT_FALSE(counter.IsActive());
}
