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

#include <gtest/gtest.h>

#include "core/memory.hpp"
#include "machine/call_stack.hpp"
#include "machine/stack_frame.hpp"

class FakeStack : public CallStack {
 public:
  StackFrame* FindTopRealFrame() const { return frame.get(); }

  std::shared_ptr<StackFrame> frame;
};

class StackRoutingTest : public ::testing::Test {
 protected:
  void SetUp() { memory.AttachCallStack(&stack); }

  FakeStack stack;
  Memory memory;
};

TEST_F(StackRoutingTest, IntL) {
  auto frame1 = std::make_shared<StackFrame>();
  auto frame2 = std::make_shared<StackFrame>();
  auto frame3 = std::make_shared<StackFrame>();

  for (int i = 0; i < 40; ++i) {
    frame1->intL.Set(i, i);
    frame2->intL.Set(i, i * 2);
    frame3->intL.Set(i, i * i);
  }

  stack.frame = frame1;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(IntBank::L, i), i);
  }
  stack.frame = frame2;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(IntBank::L, i), i * 2);
  }
  stack.frame = frame3;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(IntBank::L, i), i * i);
  }
}

TEST_F(StackRoutingTest, StrK) {
  auto frame1 = std::make_shared<StackFrame>();
  auto frame2 = std::make_shared<StackFrame>();
  auto frame3 = std::make_shared<StackFrame>();

  for (int i = 0; i < 40; ++i) {
    frame1->strK.Set(i, std::to_string(i));
    frame2->strK.Set(i, std::to_string(i * 2));
    frame3->strK.Set(i, std::to_string(i * i));
  }

  stack.frame = frame1;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(StrBank::K, i), std::to_string(i));
  }
  stack.frame = frame2;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(StrBank::K, i), std::to_string(i * 2));
  }
  stack.frame = frame3;
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(memory.Read(StrBank::K, i), std::to_string(i * i));
  }
}

TEST_F(StackRoutingTest, GetStackMemorySnapshotsCurrentFrame) {
  auto frame1 = std::make_shared<StackFrame>();
  auto frame2 = std::make_shared<StackFrame>();

  frame1->intL.Set(0, 10);
  frame1->strK.Set(0, "first");
  frame2->intL.Set(0, 20);
  frame2->strK.Set(0, "second");

  stack.frame = frame1;
  auto snapshot = memory.GetStackMemory();

  stack.frame = frame2;
  EXPECT_EQ(memory.Read(IntBank::L, 0), 20);
  EXPECT_EQ(memory.Read(StrBank::K, 0), "second");
  EXPECT_EQ(snapshot.L.Get(0), 10);
  EXPECT_EQ(snapshot.K.Get(0), "first");
}
