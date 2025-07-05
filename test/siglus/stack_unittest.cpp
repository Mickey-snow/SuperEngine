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

namespace siglus_test {

using namespace libsiglus;

// helper
inline Value operator""_s(char const* s, size_t len) {
  return Value(String(std::string(s, len)));
}
inline Value as_int(int i) { return Value(Integer(i)); }

class StackTest : public ::testing::Test {
 protected:
  Stack s;  // Create a Stack object to use in tests
};

TEST_F(StackTest, Basic) {
  {
    s.Push(as_int(10));
    EXPECT_EQ(s.Backint(), as_int(10));
    s.Push(as_int(20));
    EXPECT_EQ(s.Backint(), as_int(20));
    s.Clear();
  }

  {
    s.Push(as_int(10));
    s.Push(as_int(20));
    EXPECT_EQ(s.Popint(), as_int(20));
    EXPECT_EQ(s.Backint(), as_int(10));
    EXPECT_EQ(s.Popint(), as_int(10));
    s.Clear();
  }

  {
    s.Push("hello"_s);
    EXPECT_EQ(s.Backstr(), "hello"_s);
    s.Push("world"_s);
    EXPECT_EQ(s.Backstr(), "world"_s);
    s.Clear();
  }

  {
    s.Push("hello"_s);
    s.Push("world"_s);
    EXPECT_EQ(s.Popstr(), "world"_s);
    EXPECT_EQ(s.Backstr(), "hello"_s);
    EXPECT_EQ(s.Popstr(), "hello"_s);
    s.Clear();
  }

  {
    s.Push(as_int(1)).Push(as_int(2));
    s.Push("one"_s).Push("two"_s);
    EXPECT_EQ(s.Backint(), as_int(2));
    EXPECT_EQ(s.Backstr(), "two"_s);

    s.Popint();
    EXPECT_EQ(s.Backint(), as_int(1));
    EXPECT_EQ(s.Backstr(), "two"_s);

    s.Popstr();
    EXPECT_EQ(s.Backstr(), "one"_s);
    s.Clear();
  }

  {
    s.Push(as_int(1)).Push(as_int(2)).Push(as_int(3));
    s.Push("one"_s).Push("two"_s).Push("three"_s);
    EXPECT_EQ(s.Backint(), as_int(3));
    EXPECT_EQ(s.Backstr(), "three"_s);

    EXPECT_EQ(s.Popint(), as_int(3));
    EXPECT_EQ(s.Backint(), as_int(2));

    EXPECT_EQ(s.Popstr(), "three"_s);
    EXPECT_EQ(s.Backstr(), "two"_s);
    s.Clear();
  }
}

TEST_F(StackTest, Element) {
  using libsiglus::elm::ElementCode;
  ElementCode elm{1, 2, 3, 4};

  s.PushMarker();
  s.Push(as_int(1)).Push(as_int(2)).Push(as_int(3)).Push(as_int(4));
  EXPECT_EQ(s.Backelm(), elm);

  s.PushMarker();
  s.Push(as_int(100)).Push("garbage"_s);
  EXPECT_EQ(s.Popelm(), ElementCode{100});

  EXPECT_EQ(s.Popelm(), elm);
  EXPECT_THROW(s.Popelm(), libsiglus::StackUnderflow);
}

TEST_F(StackTest, CopyConstructor) {
  s.Push(as_int(10)).Push(as_int(20));
  s.Push("hello"_s).Push("world"_s);
  Stack s_copy = s;

  EXPECT_EQ(s_copy.Backint(), as_int(20));
  EXPECT_EQ(s_copy.Backstr(), "world"_s);

  s_copy.Popint();
  EXPECT_EQ(s_copy.Backint(), as_int(10));
  EXPECT_EQ(s.Backint(), as_int(20))
      << "Original stack should remain unchanged.";
}

TEST_F(StackTest, AssignmentOperator) {
  s.Push(as_int(10)).Push(as_int(20));
  s.Push("hello"_s).Push("world"_s);
  Stack s_copy;
  s_copy = s;

  EXPECT_EQ(s_copy.Backint(), as_int(20));
  EXPECT_EQ(s_copy.Backstr(), "world"_s);

  s_copy.Popstr();
  EXPECT_EQ(s_copy.Backstr(), "hello"_s);
  EXPECT_EQ(s.Backstr(), "world"_s)
      << "Original stack should remain unchanged.";
}

TEST_F(StackTest, MoveConstructor) {
  s.Push(as_int(10)).Push(as_int(20));
  s.Push("hello"_s).Push("world"_s);
  Stack s_moved = std::move(s);

  EXPECT_EQ(s_moved.Backint(), as_int(20));
  EXPECT_EQ(s_moved.Backstr(), "world"_s);
}

TEST_F(StackTest, MoveAssignmentOperator) {
  s.Push(as_int(10)).Push(as_int(20));
  s.Push("hello"_s).Push("world"_s);
  Stack s_moved;
  s_moved = std::move(s);

  EXPECT_EQ(s_moved.Backint(), as_int(20));
  EXPECT_EQ(s_moved.Backstr(), "world"_s);
}

TEST_F(StackTest, PushMoveString) {
  std::string str = "test";
  s.Push(String(std::move(str)));

  EXPECT_EQ(s.Backstr(), "test"_s);
  EXPECT_TRUE(str.empty() || str == "");  // After move, str may be empty
}

TEST_F(StackTest, ConstCorrectness) {
  {
    const Stack& const_s = s.Push(as_int(42));
    EXPECT_EQ(const_s.Backint(), as_int(42));
  }

  {
    const Stack& const_s = s.Push("const test"_s);
    EXPECT_EQ(const_s.Backstr(), "const test"_s);
  }
}

TEST_F(StackTest, AccessEmpty) {
  using libsiglus::StackUnderflow;
  EXPECT_THROW(s.Popint(), StackUnderflow);
  EXPECT_THROW(s.Popstr(), StackUnderflow);
  EXPECT_THROW(s.Backint(), StackUnderflow);
  EXPECT_THROW(s.Backstr(), StackUnderflow);
}

}  // namespace siglus_test
