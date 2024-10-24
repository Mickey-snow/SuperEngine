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

#include <gtest/gtest.h>

#include "utilities/clock.h"
#include "utilities/stopwatch.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace chr = std::chrono;
using timepoint_t = Clock::timepoint_t;
using duration_t = Clock::duration_t;

class FakeClock : public Clock {
 public:
  FakeClock() = default;
  ~FakeClock() = default;

  void SetTime(timepoint_t now) { now_ = now; }
  timepoint_t GetTime() const override { return now_; }

  duration_t GetTicks() const override {
    ADD_FAILURE() << "FakeClock::GetTicks() should not be called.";
    return duration_t::zero();
  }

 private:
  timepoint_t now_;
};

class StopwatchTest : public ::testing::Test {
 protected:
  struct StopwatchTestCtx {
    std::vector<std::pair<chr::milliseconds, Stopwatch::Action>> actions;
    std::vector<
        std::tuple<chr::milliseconds, Stopwatch::State, Stopwatch::duration_t>>
        checkers;

    void Doit() {
      std::vector<chr::milliseconds> keytimes;
      keytimes.reserve(actions.size() + checkers.size());
      for (const auto& it : actions)
        keytimes.push_back(it.first);
      for (const auto& it : checkers)
        keytimes.push_back(std::get<0>(it));

      std::sort(actions.begin(), actions.end());
      std::sort(checkers.begin(), checkers.end());
      std::sort(keytimes.begin(), keytimes.end());
      keytimes.erase(std::unique(keytimes.begin(), keytimes.end()),
                     keytimes.end());

      auto clock = std::make_shared<FakeClock>();
      auto epoch = std::chrono::steady_clock::now();
      clock->SetTime(epoch);
      Stopwatch stopwatch(clock);

      size_t action_idx = 0, checker_idx = 0;
      for (const auto& duration : keytimes) {
        auto now = epoch + duration;
        clock->SetTime(now);

        while (action_idx < actions.size() &&
               actions[action_idx].first == duration) {
          stopwatch.Apply(actions[action_idx++].second);
        }

        while (checker_idx < checkers.size() &&
               std::get<0>(checkers[checker_idx]) == duration) {
          auto expect_state = std::get<1>(checkers[checker_idx]);
          auto expect_anmtime =
              chr::duration_cast<Stopwatch::duration_t>(std::get<2>(
                  checkers[checker_idx]));  // Cast before EXPECT_ marco to
                                            // prevent unexpected gtest behavior
          ++checker_idx;

          EXPECT_EQ(expect_state, stopwatch.GetState())
              << "at tick " << duration.count();
          EXPECT_EQ(expect_anmtime, stopwatch.GetReading())
              << "at tick " << duration.count();
        }
      }
    }
  };

  // possible controlling actions
  inline static constexpr auto Run = Stopwatch::Action::Run,
                               Pause = Stopwatch::Action::Pause,
                               Reset = Stopwatch::Action::Reset;

  // possible stopwatch state
  inline static constexpr auto Running = Stopwatch::State::Running,
                               Paused = Stopwatch::State::Paused,
                               Set = Stopwatch::State::Stopped;
};

TEST_F(StopwatchTest, Countup) {
  using std::chrono_literals::operator""ms;

  StopwatchTestCtx testinstance;

  testinstance.actions = {{0ms, Run}, {15ms, Run}};
  testinstance.checkers = {
      {1ms, Running, 1ms},
      {10ms, Running, 10ms},
      {50ms, Running, 50ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, ToggleRun) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run},
      {12ms, Pause},
      {20ms, Pause},
      {22ms, Run},
  };
  testinstance.checkers = {{0ms, Running, 0ms},
                           {11ms, Running, 11ms},
                           {20ms, Paused, 12ms},
                           {32ms, Running, 22ms}};

  testinstance.Doit();
}

TEST_F(StopwatchTest, StopReset) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {5ms, Run}, {12ms, Reset}, {15ms, Pause}, {20ms, Run}};
  testinstance.checkers = {
      {0ms, Paused, 0ms},
      {11ms, Running, 7ms},
      {13ms, Set, 0ms},
      {16ms, Set,
       0ms},  // Pause action should be ignored when at Finished state
      {32ms, Running, 12ms}};
}

TEST_F(StopwatchTest, StopWhenAlreadyStopped) {
  // Test applying Stop action when already in Finished state.
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Reset},
      {10ms, Reset},
  };

  testinstance.checkers = {
      {5ms, Set, 0ms},
      {15ms, Set, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, RapidTransitions) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run}, {1ms, Pause}, {2ms, Run}, {3ms, Pause}, {4ms, Run},
  };

  testinstance.checkers = {
      {0ms, Running, 0ms}, {1ms, Paused, 1ms},  {2ms, Running, 1ms},
      {3ms, Paused, 2ms},  {4ms, Running, 2ms}, {5ms, Running, 3ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, NoActions) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.checkers = {
      {0ms, Paused, 0ms},
      {10ms, Paused, 0ms},
      {20ms, Paused, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, Interrupt) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run},
      {0ms, Reset},
  };

  testinstance.checkers = {
      {0ms, Set, 0ms},
      {10ms, Set, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, LongDuration) {
  using namespace std::chrono_literals;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run},
      {5000h, Pause},
      {7000h, Run},
      {10000h, Reset},
  };

  testinstance.checkers = {
      {2500h, Running, 2500h},
      {6000h, Paused, 5000h},
      {8000h, Running, 6000h},
      {11000h, Set, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, MultipleToggles) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run},     {100ms, Pause}, {200ms, Run},
      {300ms, Pause}, {400ms, Run},   {500ms, Reset},
  };

  testinstance.checkers = {
      {50ms, Running, 50ms},  {150ms, Paused, 100ms},  {250ms, Running, 150ms},
      {350ms, Paused, 200ms}, {450ms, Running, 250ms}, {550ms, Set, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, TypicalControl) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      {0ms, Run},
      {5ms, Pause},
      {10ms, Run},
      {15ms, Reset},
  };

  testinstance.checkers = {
      {3ms, Running, 3ms},
      {8ms, Paused, 5ms},
      {12ms, Running, 7ms},
      {18ms, Set, 0ms},
  };

  testinstance.Doit();
}

TEST_F(StopwatchTest, ReadFromInit) {
  using std::chrono_literals::operator""ms;
  StopwatchTestCtx testinstance;

  testinstance.actions = {
      // none
  };

  testinstance.checkers = {{0ms, Paused, 0ms}};

  testinstance.Doit();
}

TEST_F(StopwatchTest, BrokenClock) {
  using std::chrono_literals::operator""ms;

  auto clock = std::make_shared<FakeClock>();
  auto epoch = std::chrono::steady_clock::now();
  clock->SetTime(epoch);

  Stopwatch stopwatch(clock);
  stopwatch.Apply(Run);

  clock->SetTime(epoch + 10ms);  // Advance time
  EXPECT_EQ(stopwatch.GetReading(), 10ms);

  clock->SetTime(epoch + 5ms);  // Move time backward
  EXPECT_THROW(stopwatch.GetReading(), std::runtime_error);
}
