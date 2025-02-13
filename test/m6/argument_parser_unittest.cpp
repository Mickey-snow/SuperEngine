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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "util.hpp"

#include "m6/argparse.hpp"
#include "m6/value.hpp"
#include "m6/value_internal/int.hpp"
#include "m6/value_internal/str.hpp"

using namespace m6;

TEST(ArgparseTest, DirectValues) {
  auto [v1, v2, v3, v4] =
      ParseArgs<int, std::shared_ptr<Int>, std::shared_ptr<String>, Value>(
          {make_value(1), make_value(2), make_value("3"), make_value(false)});
  EXPECT_EQ(v1, 1);
  EXPECT_VALUE_EQ(v2, 2);
  EXPECT_VALUE_EQ(v3, "3");
  EXPECT_VALUE_EQ(v4, 0);
}

TEST(ArgparseTest, Ints) {
  auto [v1, v2, v3] =
      ParseArgs<int, int, int>({make_value(1), make_value(2), make_value(3)});

  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);
  EXPECT_EQ(v3, 3);
}

TEST(ArgparseTest, Strings) {
  auto [s1, s2] = ParseArgs<std::string, std::string>(
      {make_value("hello"), make_value("world")});
  EXPECT_EQ(s1, "hello");
  EXPECT_EQ(s2, "world");
}

TEST(ArgparseTest, Intrefs) {
  auto value1 = make_value(123);
  auto value2 = make_value(321);
  auto [v1, v2] = ParseArgs<int*, int*>({value1, value2});

  *v1 = 1;
  *v2 = 2;
  EXPECT_VALUE_EQ(value1, 1);
  EXPECT_VALUE_EQ(value2, 2);
}

TEST(ArgparseTest, Strrefs) {
  auto value1 = make_value("first");
  auto value2 = make_value("second");
  auto [s1, s2] = ParseArgs<std::string*, std::string*>({value1, value2});

  *s1 = "foo";
  *s2 = "boo";
  EXPECT_VALUE_EQ(value1, "foo");
  EXPECT_VALUE_EQ(value2, "boo");
}

TEST(ArgparseTest, Optional) {
  auto [s1, s2, s3, s4] =
      ParseArgs<int, std::optional<int>, std::optional<std::string>,
                std::optional<std::string>>({make_value(1), make_value("two")});

  EXPECT_FALSE(s2.has_value());
  EXPECT_EQ(s3, "two");
  EXPECT_FALSE(s4.has_value());
}

TEST(ArgparseTest, Arglist) {
  auto [first, remain] = ParseArgs<std::string, std::vector<int>>(
      {make_value("sum"), make_value(1), make_value(2), make_value(3),
       make_value(4)});

  EXPECT_EQ(first, "sum");
  EXPECT_EQ(remain, (std::vector<int>{1, 2, 3, 4}));
}

TEST(ArgparseTest, InsufficientArguments) {
  EXPECT_THROW(
      { [[maybe_unused]] auto [v] = ParseArgs<int>({}); }, SyntaxError)
      << "Insufficient arguments for a non-optional parameter should throw "
         "SyntaxError";
}

TEST(ArgparseTest, TooManyArguments) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto [v] =
            ParseArgs<int>({make_value(1), make_value(1)});
      },
      SyntaxError);
}

TEST(ArgparseTest, TypeMismatch) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto [v] = ParseArgs<int>({make_value("not an int")});
      },
      TypeError)
      << "Type mismatch for a non-optional int should throw TypeError";
}

TEST(ArgparseTest, OptionalMismatch) {
  auto [optInt, v1] =
      ParseArgs<std::optional<int>, std::string>({make_value("not an int")});
  EXPECT_FALSE(optInt.has_value())
      << "When an optional int parameter receives an argument of the wrong "
         "type, it returns nullopt";
}

TEST(ArgparseTest, VectorTypeMismatch) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto [vec] = ParseArgs<std::vector<int>>(
            {make_value(1), make_value("bad"), make_value(3)});
      },
      TypeError)
      << "A vector parameter should throw TypeError if any element fails to "
         "convert";
}

TEST(ArgparseTest, EmptyVector) {
  auto [cmd, vec] =
      ParseArgs<std::string, std::vector<int>>({make_value("cmd")});
  EXPECT_EQ(cmd, "cmd");
  EXPECT_TRUE(vec.empty()) << "A vector parameter with no corresponding "
                              "arguments should yield an empty vector";
}

TEST(ArgparseTest, PointerMismatch) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto [ptr] =
            ParseArgs<int*>({make_value("not an int")});
      },
      TypeError);
}
