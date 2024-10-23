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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "object/drawer/anm.h"
#include "object/drawer/file.h"
#include "object/drawer/gan.h"
#include "object/service_locator.h"
#include "systems/base/surface.h"
#include "test_system/mock_surface.h"

#include <chrono>
#include <utility>
#include <vector>

class MockedLocator : public IRenderingService {
 public:
  MockedLocator() = default;

  MOCK_METHOD((unsigned int), GetTicks, (), (const override));
  MOCK_METHOD(void, MarkObjStateDirty, (), (const override));
  MOCK_METHOD(void, MarkScreenDirty, (GraphicsUpdateType type), (override));
};

namespace chr = std::chrono;
using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtMost;
using ::testing::Return;

class AnimatorTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockedLocator> CreateLocator() {
    auto locator = std::make_shared<MockedLocator>();

    EXPECT_CALL(*locator, MarkObjStateDirty()).Times(AnyNumber());
    EXPECT_CALL(*locator, MarkScreenDirty(_)).Times(AnyNumber());
    EXPECT_CALL(*locator, GetTicks()).Times(0);
    return locator;
  }

  std::shared_ptr<MockedSurface> CreateAnimation(int num_frames) {
    auto surface = std::make_shared<MockedSurface>();

    EXPECT_CALL(*surface, GetNumPatterns())
        .Times(AnyNumber())
        .WillRepeatedly(Return(num_frames));

    return surface;
  }
};

class ObjOfFileAnimatorTest : public AnimatorTest {
 protected:
  enum ControllerAction { Pause, Resume, Stop, LoopOn, LoopOff };
  struct AnimationTestCtx {
    std::shared_ptr<GraphicsObjectOfFile> obj;
    std::vector<std::pair<chr::milliseconds, ControllerAction>> actions;
    std::vector<
        std::pair<chr::milliseconds,
                  std::function<void(std::shared_ptr<GraphicsObjectOfFile>)>>>
        checkers;
  };

  void TestAnimation(AnimationTestCtx ctx) {
    ctx.obj->time_at_last_frame_change_ = 0;

    std::vector<chr::milliseconds> keytimes;
    keytimes.reserve(ctx.actions.size() + ctx.checkers.size());
    for (const auto& it : ctx.actions)
      keytimes.push_back(it.first);
    for (const auto& it : ctx.checkers)
      keytimes.push_back(it.first);

    std::sort(keytimes.begin(), keytimes.end());
    keytimes.erase(std::unique(keytimes.begin(), keytimes.end()),
                   keytimes.end());
    size_t action_idx = 0, checker_idx = 0;
    for (const auto& now : keytimes) {
      while (action_idx < ctx.actions.size() &&
             ctx.actions[action_idx].first == now) {
        PerformAction(ctx.obj, ctx.actions[action_idx++].second);
      }
      ctx.obj->Execute(static_cast<unsigned int>(now.count()));

      while (checker_idx < ctx.checkers.size() &&
             ctx.checkers[checker_idx].first == now) {
        std::invoke(ctx.checkers[checker_idx++].second, ctx.obj);
      }
    }
  }

  void PerformAction(std::shared_ptr<GraphicsObjectOfFile> obj,
                     ControllerAction action) {
    auto animator = obj->GetAnimator();
    switch (action) {
      case Pause:
        animator->SetIsPlaying(false);
        break;
      case Resume:
        animator->SetIsPlaying(true);
        break;
      case Stop:
        animator->SetIsFinished(true);
        break;
      case LoopOn:
        animator->SetAfterAction(AFTER_LOOP);
        break;
      case LoopOff:
        animator->SetAfterAction(AFTER_NONE);
        break;
      default:
        ADD_FAILURE() << "AnimatorTest: unknown action "
                      << std::to_string(action);
    }
  }
};

TEST_F(ObjOfFileAnimatorTest, OnepassAnimation) {
  auto locator = CreateLocator();
  auto surface = CreateAnimation(3);
  auto obj = std::make_shared<GraphicsObjectOfFile>(locator, surface);
  obj->current_frame_ = 0;
  obj->frame_time_ = 10;

  using std::chrono_literals::operator""ms;

  AnimationTestCtx ctx;
  ctx.obj = obj;
  ctx.actions = {
      {std::make_pair(0ms, Resume)},
  };

  ctx.checkers = {
      {std::make_pair(1ms,
                      [](auto obj) {
                        auto animator = obj->GetAnimator();
                        EXPECT_EQ(obj->current_frame_, 0);
                        EXPECT_TRUE(animator->IsPlaying());
                        EXPECT_FALSE(animator->IsFinished());
                      })},
      {std::make_pair(12ms,
                      [](auto obj) { EXPECT_EQ(obj->current_frame_, 1); })},
      {std::make_pair(50ms,
                      [](auto obj) {
                        auto animator = obj->GetAnimator();
                        EXPECT_EQ(obj->current_frame_, 2)
                            << "Animator should revert to the last frame at "
                               "the end of animation";
                        EXPECT_FALSE(animator->IsPlaying());
                        EXPECT_TRUE(animator->IsFinished());
                      })},
  };

  TestAnimation(std::move(ctx));
}

TEST_F(ObjOfFileAnimatorTest, StaticFrame) {
  auto locator = CreateLocator();
  auto surface = CreateAnimation(0);
  auto obj = std::make_shared<GraphicsObjectOfFile>(locator, surface);

  // Verify that the object is not considered an animation, currently broken
  EXPECT_FALSE(obj->IsAnimation());
  EXPECT_EQ(obj->GetAnimator(), nullptr)
      << "An animator should not be created when there's no animation frame.";

  obj->frame_time_ = 10;
  obj->current_frame_ = 0;
  obj->time_at_last_frame_change_ = 0;

  using namespace std::chrono_literals;

  AnimationTestCtx ctx;
  ctx.obj = obj;

  ctx.actions = {};
  ctx.checkers = {
      {0ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 0)
             << "Current frame should remain at 0 at time 0ms";
       }},
      {100ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 0)
             << "Current frame should remain at 0 at time 100ms";
       }},
  };

  TestAnimation(std::move(ctx));
}

TEST_F(ObjOfFileAnimatorTest, TogglePlaying) {
  auto locator = CreateLocator();
  auto surface = CreateAnimation(3);  // 3 frames
  auto obj = std::make_shared<GraphicsObjectOfFile>(locator, surface);

  auto animator = obj->GetAnimator();
  ASSERT_NE(animator, nullptr);
  animator->SetIsPlaying(true);

  obj->frame_time_ = 10;
  obj->current_frame_ = 0;
  obj->time_at_last_frame_change_ = 0;

  using namespace std::chrono_literals;

  AnimationTestCtx ctx;
  ctx.obj = obj;

  ctx.actions = {
      {12ms, Pause},
      {22ms, Resume},
  };
  ctx.checkers = {
      {0ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 0)
             << "Current frame should be 0 at time 0ms";
       }},
      {11ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 1)
             << "Current frame should be 1 at time before pausing";
       }},
      {20ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 1)
             << "Current frame should remain at 1 while paused";
       }},
      {32ms,
       [](auto obj) {
         EXPECT_EQ(obj->current_frame_, 2)
             << "Current frame should advance to 2 after resuming at 22ms";
       }},
  };

  TestAnimation(std::move(ctx));
}
