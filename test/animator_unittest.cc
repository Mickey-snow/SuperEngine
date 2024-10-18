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

class MockedSurface : public Surface {
 public:
  MockedSurface() = default;
  ~MockedSurface() override = default;

  MOCK_METHOD(void, EnsureUploaded, (), (const, override));
  MOCK_METHOD(void, Fill, (const RGBAColour& colour), (override));
  MOCK_METHOD(void,
              Fill,
              (const RGBAColour& colour, const Rect& area),
              (override));
  MOCK_METHOD(void,
              ToneCurve,
              (const ToneCurveRGBMap effect, const Rect& area),
              (override));
  MOCK_METHOD(void, Invert, (const Rect& area), (override));
  MOCK_METHOD(void, Mono, (const Rect& area), (override));
  MOCK_METHOD(void,
              ApplyColour,
              (const RGBColour& colour, const Rect& area),
              (override));
  MOCK_METHOD(void, SetIsMask, (bool is), (override));
  MOCK_METHOD(Size, GetSize, (), (const, override));
  MOCK_METHOD(void, Dump, (), (override));
  MOCK_METHOD(void,
              BlitToSurface,
              (Surface & dest_surface,
               const Rect& src,
               const Rect& dst,
               int alpha,
               bool use_src_alpha),
              (const, override));
  MOCK_METHOD(void,
              RenderToScreen,
              (const Rect& src, const Rect& dst, int alpha),
              (const, override));
  MOCK_METHOD(void,
              RenderToScreen,
              (const Rect& src, const Rect& dst, const int opacity[4]),
              (const, override));
  MOCK_METHOD(
      void,
      RenderToScreenAsColorMask,
      (const Rect& src, const Rect& dst, const RGBAColour& colour, int filter),
      (const, override));
  MOCK_METHOD(
      void,
      RenderToScreenAsObject,
      (const GraphicsObject& rp, const Rect& src, const Rect& dst, int alpha),
      (const, override));
  MOCK_METHOD(int, GetNumPatterns, (), (const, override));
  MOCK_METHOD(const GrpRect&, GetPattern, (int patt_no), (const, override));
  MOCK_METHOD(void,
              GetDCPixel,
              (const Point& pos, int& r, int& g, int& b),
              (const, override));
  MOCK_METHOD(std::shared_ptr<Surface>,
              ClipAsColorMask,
              (const Rect& clip_rect, int r, int g, int b),
              (const, override));
  MOCK_METHOD(Surface*, Clone, (), (const, override));
};

class MockedLocator : public IRenderingService {
 public:
  MockedLocator() = default;

  MOCK_METHOD((unsigned int), GetTicks, (), (const override));
  MOCK_METHOD(void, MarkObjStateDirty, (), (const override));
  MOCK_METHOD(void, MarkScreenDirty, (GraphicsUpdateType type), (override));
};

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtMost;
using ::testing::Return;

class AnimatorTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockedLocator> CreateTimer(std::vector<unsigned int> ticks) {
    auto locator = std::make_shared<MockedLocator>();

    EXPECT_CALL(*locator, MarkObjStateDirty()).Times(AnyNumber());
    EXPECT_CALL(*locator, MarkScreenDirty(_)).Times(AnyNumber());

    auto& timer = EXPECT_CALL(*locator, GetTicks())
                      .Times(::testing::AtMost(ticks.size()));
    for (const auto& it : ticks)
      timer.WillOnce(Return(it));
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

class ObjOfFileAnimatorTest : public AnimatorTest {};

TEST_F(ObjOfFileAnimatorTest, OnepassAnimation) {
  auto locator = CreateTimer({0, 12, 50});
  auto surface = CreateAnimation(3);

  GraphicsObjectOfFile obj(locator, surface);
  auto animator = obj.GetAnimator();
  animator->SetIsPlaying(true);

  obj.frame_time_ = 10;
  obj.current_frame_ = 0;
  obj.time_at_last_frame_change_ = 0;

  obj.Execute();  // at tick 0
  EXPECT_EQ(obj.current_frame_, 0);
  EXPECT_TRUE(animator->IsPlaying());
  EXPECT_FALSE(animator->IsFinished());

  obj.Execute();  // at tick 12
  EXPECT_EQ(obj.current_frame_, 1);

  obj.Execute();  // at tick 50
  EXPECT_EQ(obj.current_frame_, 2)
      << "Animator should revert to the last frame at the end of animation";

  EXPECT_FALSE(animator->IsPlaying())
      << "Animator should stop playing after reverting to the first frame";
  EXPECT_TRUE(animator->IsFinished())
      << "Animator should be marked as finished after reverting";
}

TEST_F(ObjOfFileAnimatorTest, StaticFrame) {
  auto locator = CreateTimer({0, 20});
  auto surface = CreateAnimation(0);  // 0 frames

  GraphicsObjectOfFile obj(locator, surface);
  auto animator = obj.GetAnimator();
  animator->SetIsPlaying(true);

  obj.frame_time_ = 10;
  obj.current_frame_ = 0;
  obj.time_at_last_frame_change_ = 0;

  EXPECT_FALSE(obj.IsAnimation());
  EXPECT_EQ(obj.GetAnimator(), nullptr)
      << "An animator should not be created when there's no animation frame.";

  obj.Execute();  // at tick 0
  obj.Execute();  // at tick 10
  EXPECT_EQ(obj.current_frame_, 0);
}

TEST_F(AnimatorTest, TogglePlaying) {
  auto locator = CreateTimer({0, 12, 22, 32});
  auto surface = CreateAnimation(3);  // 3 frames

  GraphicsObjectOfFile obj(locator, surface);
  auto animator = obj.GetAnimator();
  ASSERT_NE(animator, nullptr);
  animator->SetIsPlaying(true);

  obj.frame_time_ = 10;
  obj.current_frame_ = 0;
  obj.time_at_last_frame_change_ = 0;

  obj.Execute();  // at tick 0
  EXPECT_EQ(obj.current_frame_, 0);

  animator->SetIsPlaying(false);  // Pause
  obj.Execute();                  // at tick 12
  EXPECT_EQ(locator->GetTicks(), 12)
      << "GetTicks should not be called when animation paused";
  EXPECT_EQ(obj.current_frame_, 0)
      << "Animator should not advance frames when paused";

  animator->SetIsPlaying(true);  // Resume
  obj.Execute();                 // at tick 22
  EXPECT_EQ(obj.current_frame_, 2)
      << "Animator should resume advancing frames when playing";
}
