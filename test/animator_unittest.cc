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

#include "object/animator.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace chr = std::chrono;
using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;

class AnimatorTest : public ::testing::Test {
 protected:
  struct AnimatorTestCtx {
    std::shared_ptr<Animator> animator;
    std::vector<std::pair<chr::milliseconds, Animator::Action>> actions;
    std::vector<
        std::tuple<chr::milliseconds, Animator::State, Animator::duration_t>>
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

      auto epoch = std::chrono::steady_clock::now();
      size_t action_idx = 0, checker_idx = 0;
      animator->Notify(epoch);
      for (const auto& duration : keytimes) {
        auto now = epoch + duration;

        while (action_idx < actions.size() &&
               actions[action_idx].first == duration) {
          animator->Apply(actions[action_idx++].second, now);
        }
        animator->Notify(now);

        while (checker_idx < checkers.size() &&
               std::get<0>(checkers[checker_idx]) == duration) {
          auto expect_state = std::get<1>(checkers[checker_idx]);
          auto expect_anmtime =
              chr::duration_cast<Animator::duration_t>(std::get<2>(
                  checkers[checker_idx]));  // Cast before EXPECT_ marco to
                                            // prevent unexpected gtest behavior
          ++checker_idx;

          EXPECT_EQ(expect_state, animator->GetState())
              << "at tick " << duration.count();
          EXPECT_EQ(expect_anmtime, animator->GetAnmTime())
              << "at tick " << duration.count();
        }
      }
    }
  };

  // possible controlling actions
  inline static constexpr auto Play = Animator::Action::Play,
                               Pause = Animator::Action::Pause,
                               Stop = Animator::Action::Stop;

  // possible animator state
  inline static constexpr auto Playing = Animator::State::Playing,
                               Paused = Animator::State::Paused,
                               Finished = Animator::State::Finished;
};

TEST_F(AnimatorTest, OnepassAnimation) {
  using std::chrono_literals::operator""ms;

  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();
  testinstance.actions = {
      {0ms, Play},
      {15ms, Play}
  };
  testinstance.checkers = {
      {1ms, Playing, 1ms},
      {10ms, Playing, 10ms},
      {50ms, Playing, 50ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, TogglePlaying) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
      {0ms, Play},
      {12ms, Pause},
      {20ms, Pause},
      {22ms, Play},
  };
  testinstance.checkers = {{0ms, Playing, 0ms},
                           {11ms, Playing, 11ms},
                           {20ms, Paused, 12ms},
                           {32ms, Playing, 22ms}};

  testinstance.Doit();
}

TEST_F(AnimatorTest, StopReset) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
      {5ms, Play}, {12ms, Stop}, {15ms, Pause}, {20ms, Play}};
  testinstance.checkers = {
      {0ms, Paused, 0ms},
      {11ms, Playing, 7ms},
      {13ms, Finished, 0ms},
      {16ms, Finished,
       0ms},  // Pause action should be ignored when at Finished state
      {32ms, Playing, 12ms}};
}

TEST_F(AnimatorTest, StopWhenAlreadyStopped) {
  // Test applying Stop action when already in Finished state.
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Stop},
    {10ms, Stop},
  };

  testinstance.checkers = {
    {5ms, Finished, 0ms},
    {15ms, Finished, 0ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, RapidTransitions) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Play},
    {1ms, Pause},
    {2ms, Play},
    {3ms, Pause},
    {4ms, Play},
  };

  testinstance.checkers = {
    {0ms, Playing, 0ms},
    {1ms, Paused, 1ms},
    {2ms, Playing, 1ms},
    {3ms, Paused, 2ms},
    {4ms, Playing, 2ms},
    {5ms, Playing, 3ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, NoActions) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.checkers = {
    {0ms, Paused, 0ms},
    {10ms, Paused, 0ms},
    {20ms, Paused, 0ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, Interrupt) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Play},
    {0ms, Stop},
  };

  testinstance.checkers = {
    {0ms, Finished, 0ms},
    {10ms, Finished, 0ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, LongDuration) {
  using namespace std::chrono_literals;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Play},
    {5000h, Pause},
    {7000h, Play},
    {10000h, Stop},
  };

  testinstance.checkers = {
    {2500h, Playing, 2500h},
    {6000h, Paused, 5000h},
    {8000h, Playing, 6000h},
    {11000h, Finished, 0ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, MultipleToggles) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Play},
    {100ms, Pause},
    {200ms, Play},
    {300ms, Pause},
    {400ms, Play},
    {500ms, Stop},
  };

  testinstance.checkers = {
    {50ms, Playing, 50ms},
    {150ms, Paused, 100ms},
    {250ms, Playing, 150ms},
    {350ms, Paused, 200ms},
    {450ms, Playing, 250ms},
    {550ms, Finished, 0ms},
  };

  testinstance.Doit();
}

TEST_F(AnimatorTest, TypicalAnimation) {
  using std::chrono_literals::operator""ms;
  AnimatorTestCtx testinstance;
  testinstance.animator = std::make_shared<Animator>();

  testinstance.actions = {
    {0ms, Play},
    {5ms, Pause},
    {10ms, Play},
    {15ms, Stop},
  };

  testinstance.checkers = {
    {3ms, Playing, 3ms},
    {8ms, Paused, 5ms},
    {12ms, Playing, 7ms},
    {18ms, Finished, 0ms},
  };

  testinstance.Doit();
}
