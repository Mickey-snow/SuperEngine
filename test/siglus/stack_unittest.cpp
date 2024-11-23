// -----------------------------------------------------------------------
//
// This file is part of RLVM
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

#include "libsiglus/stack.hpp"

using libsiglus::Stack;

class StackTest : public ::testing::Test {
 protected:
  Stack s;  // Create a Stack object to use in tests
};

TEST_F(StackTest, Basic) {
  {
    s.Push(10);
    EXPECT_EQ(s.Backint(), 10);
    s.Push(20);
    EXPECT_EQ(s.Backint(), 20);
    s.Clear();
  }

  {
    s.Push(10);
    s.Push(20);
    EXPECT_EQ(s.Popint(), 20);
    EXPECT_EQ(s.Backint(), 10);
    EXPECT_EQ(s.Popint(), 10);
    s.Clear();
  }

  {
    s.Push("hello");
    EXPECT_EQ(s.Backstr(), "hello");
    s.Push("world");
    EXPECT_EQ(s.Backstr(), "world");
    s.Clear();
  }

  {
    s.Push("hello");
    s.Push("world");
    EXPECT_EQ(s.Popstr(), "world");
    EXPECT_EQ(s.Backstr(), "hello");
    EXPECT_EQ(s.Popstr(), "hello");
    s.Clear();
  }

  {
    s.Push(1).Push(2);
    s.Push("one").Push("two");
    EXPECT_EQ(s.Backint(), 2);
    EXPECT_EQ(s.Backstr(), "two");

    s.Popint();
    EXPECT_EQ(s.Backint(), 1);
    EXPECT_EQ(s.Backstr(), "two");

    s.Popstr();
    EXPECT_EQ(s.Backstr(), "one");
    s.Clear();
  }

  {
    s.Push(1).Push(2).Push(3);
    s.Push("one").Push("two").Push("three");
    EXPECT_EQ(s.Backint(), 3);
    EXPECT_EQ(s.Backstr(), "three");

    EXPECT_EQ(s.Popint(), 3);
    EXPECT_EQ(s.Backint(), 2);

    EXPECT_EQ(s.Popstr(), "three");
    EXPECT_EQ(s.Backstr(), "two");
    s.Clear();
  }
}

TEST_F(StackTest, Element) {
  using libsiglus::ElementCode;
  ElementCode elm{1, 2, 3, 4};

  s.PushMarker();
  s.Push(1).Push(2).Push(3).Push(4);
  EXPECT_EQ(s.Backelm(), elm);

  s.PushMarker();
  s.Push(100).Push("garbage");
  EXPECT_EQ(s.Popelm(), ElementCode{100});
  s.Push(elm);
  EXPECT_EQ(s.Popelm(), elm);
  EXPECT_EQ(s.Popelm(), elm);

  EXPECT_THROW(s.Popelm(), libsiglus::StackUnderflow);
}

TEST_F(StackTest, CopyConstructor) {
  s.Push(10).Push(20);
  s.Push("hello").Push("world");
  Stack s_copy = s;

  EXPECT_EQ(s_copy.Backint(), 20);
  EXPECT_EQ(s_copy.Backstr(), "world");

  s_copy.Popint();
  EXPECT_EQ(s_copy.Backint(), 10);
  EXPECT_EQ(s.Backint(), 20) << "Original stack should remain unchanged.";
}

TEST_F(StackTest, AssignmentOperator) {
  s.Push(10).Push(20);
  s.Push("hello").Push("world");
  Stack s_copy;
  s_copy = s;

  EXPECT_EQ(s_copy.Backint(), 20);
  EXPECT_EQ(s_copy.Backstr(), "world");

  s_copy.Popstr();
  EXPECT_EQ(s_copy.Backstr(), "hello");
  EXPECT_EQ(s.Backstr(), "world") << "Original stack should remain unchanged.";
}

TEST_F(StackTest, MoveConstructor) {
  s.Push(10).Push(20);
  s.Push("hello").Push("world");
  Stack s_moved = std::move(s);

  EXPECT_EQ(s_moved.Backint(), 20);
  EXPECT_EQ(s_moved.Backstr(), "world");
}

TEST_F(StackTest, MoveAssignmentOperator) {
  s.Push(10).Push(20);
  s.Push("hello").Push("world");
  Stack s_moved;
  s_moved = std::move(s);

  EXPECT_EQ(s_moved.Backint(), 20);
  EXPECT_EQ(s_moved.Backstr(), "world");
}

TEST_F(StackTest, PushMoveString) {
  std::string str = "test";
  s.Push(std::move(str));

  EXPECT_EQ(s.Backstr(), "test");
  EXPECT_TRUE(str.empty() || str == "");  // After move, str may be empty
}

TEST_F(StackTest, ConstCorrectness) {
  {
    const Stack& const_s = s.Push(42);
    EXPECT_EQ(const_s.Backint(), 42);
  }

  {
    const Stack& const_s = s.Push("const test");
    EXPECT_EQ(const_s.Backstr(), "const test");
  }
}

TEST_F(StackTest, AccessEmpty) {
  using libsiglus::StackUnderflow;
  EXPECT_THROW(s.Popint(), StackUnderflow);
  EXPECT_THROW(s.Popstr(), StackUnderflow);
  EXPECT_THROW(s.Backint(), StackUnderflow);
  EXPECT_THROW(s.Backstr(), StackUnderflow);
}
