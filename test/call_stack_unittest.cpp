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
//
// -----------------------------------------------------------------------

#include "machine/call_stack.hpp"
#include "machine/stack_frame.hpp"

#include <gtest/gtest.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <format>

class CallStackTest : public ::testing::Test {
 protected:
  StackFrame MakeFrame(StackFrame::FrameType type) {
    auto frame = StackFrame();
    frame.frame_type = type;
    return frame;
  }

  auto desc(const StackFrame& frame) -> std::string {
    return std::format("({},{}) {}", frame.pos.scenario_id_, frame.pos.offset_,
                       static_cast<int>(frame.frame_type));
  }

  CallStack stack;
};

TEST_F(CallStackTest, PushPop) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_FARCALL));

  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_FARCALL);
  stack.Pop();
  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_ROOT);
  stack.Pop();
  EXPECT_EQ(stack.Top(), nullptr);
}

TEST_F(CallStackTest, FindNotLongopFrame) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_GOSUB));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  stack.Push(MakeFrame(StackFrame::TYPE_FARCALL));

  EXPECT_EQ(stack.FindTopRealFrame()->frame_type, StackFrame::TYPE_FARCALL);
  stack.Pop();
  EXPECT_EQ(stack.FindTopRealFrame()->frame_type, StackFrame::TYPE_GOSUB);
}

TEST_F(CallStackTest, CreateCopy) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  stack.Push(MakeFrame(StackFrame::TYPE_FARCALL));

  auto duplicated = stack.Clone();
  duplicated.Push(MakeFrame(StackFrame::TYPE_GOSUB));

  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_FARCALL);
  stack.Pop();
  stack = duplicated.Clone();
  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_GOSUB);
}

TEST_F(CallStackTest, LockStack) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));

  {
    auto lock = stack.GetLock();

    stack.Pop();
    EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_LONGOP);
  }

  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_ROOT);
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  {
    auto lock = stack.GetLock();

    stack.Push(MakeFrame(StackFrame::TYPE_GOSUB));
    EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_LONGOP);
  }
  EXPECT_EQ(stack.Top()->frame_type, StackFrame::TYPE_GOSUB);
}

TEST_F(CallStackTest, DoubleLock) {
  {
    auto lock = stack.GetLock();
    EXPECT_THROW(stack.GetLock(), std::logic_error);
  }
}

TEST_F(CallStackTest, CopyLockedStack) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  {
    auto lock = stack.GetLock();

    stack.Pop();
    // suppose this long operation asks the machine to create a savepoint copy
    EXPECT_THROW(stack.Clone(), std::runtime_error);
  }
}

TEST_F(CallStackTest, StackSize) {
  stack.Push(MakeFrame(StackFrame::TYPE_ROOT));
  stack.Push(MakeFrame(StackFrame::TYPE_LONGOP));
  EXPECT_EQ(stack.Size(), 2);
}

TEST_F(CallStackTest, Serialization) {
  using libreallive::ScriptLocation;
  std::stringstream ss;

  {
    StackFrame frame1(ScriptLocation(1, 10), StackFrame::TYPE_ROOT);
    frame1.strK.Set(2, "root");
    StackFrame frame2(ScriptLocation(1, 20), StackFrame::TYPE_GOSUB);
    StackFrame frame3(ScriptLocation(2, 20), StackFrame::TYPE_FARCALL);
    frame3.strK.Set(2, "hello ");
    frame3.strK.Set(3, "world");
    StackFrame frame4(ScriptLocation(2, 20), StackFrame::TYPE_LONGOP);

    stack.Push(std::move(frame1));
    stack.Push(std::move(frame2));
    stack.Push(std::move(frame3));
    stack.Push(std::move(frame4));

    boost::archive::text_oarchive oa(ss);
    oa << stack;
  }

  {
    CallStack deserialized;
    boost::archive::text_iarchive ia(ss);
    ia >> deserialized;

    StackFrame* frame3 = deserialized.FindTopRealFrame();
    EXPECT_EQ(frame3->strK.Get(2) + frame3->strK.Get(3), "hello world");
    EXPECT_EQ(desc(*deserialized.Top()), "(2,20) 3");
    EXPECT_EQ(desc(*frame3), "(2,20) 2");
    deserialized.Pop();
    deserialized.Pop();
    EXPECT_EQ(desc(*deserialized.Top()), "(1,20) 1");
    deserialized.Pop();
    EXPECT_EQ(desc(*deserialized.Top()), "(1,10) 0");
  }
}
