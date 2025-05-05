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

#include "m6/argparse.hpp"
#include "vm/value.hpp"

#include <string>

using std::string_literals::operator""s;

using namespace m6;
using serilang::Value;

template <typename... Ts>
inline auto quick_parse(auto... args) {
  std::vector<Value> arr{std::move(args)...};
  return ParseArgs<Ts...>(arr.begin(), arr.end());
}

TEST(ArgparseTest, Ints) {
  auto [v1, v2, v3] = quick_parse<int, int, int>(Value(1), Value(2), Value(3));

  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);
  EXPECT_EQ(v3, 3);
}

TEST(ArgparseTest, Strings) {
  auto [s1, s2] =
      quick_parse<std::string, std::string>(Value("hello"), Value("world"));
  EXPECT_EQ(s1, "hello");
  EXPECT_EQ(s2, "world");
}

TEST(ArgparseTest, Intrefs) {
  std::vector<Value> vals{Value(123), Value(321)};
  auto [v1, v2] = ParseArgs<int*, int*>(vals.begin(), vals.end());

  *v1 = 1;
  *v2 = 2;
  EXPECT_EQ(vals[0], 1) << vals[0].Desc();
  EXPECT_EQ(vals[1], 2) << vals[1].Desc();
}

TEST(ArgparseTest, Strrefs) {
  std::vector<Value> vals{Value("first"), Value("second")};
  auto [s1, s2] =
      ParseArgs<std::string*, std::string*>(vals.begin(), vals.end());

  *s1 = "foo";
  *s2 = "boo";
  EXPECT_EQ(vals[0], "foo"s) << vals[0].Desc();
  EXPECT_EQ(vals[1], "boo"s) << vals[1].Desc();
}

TEST(ArgparseTest, Optional) {
  auto [s1, s2, s3, s4] =
      quick_parse<int, std::optional<int>, std::optional<std::string>,
                  std::optional<std::string>>(Value(1), Value("two"));

  EXPECT_FALSE(s2.has_value());
  EXPECT_EQ(s3, "two");
  EXPECT_FALSE(s4.has_value());
}

TEST(ArgparseTest, Arglist) {
  auto [first, remain] = quick_parse<std::string, std::vector<int>>(
      Value("sum"), Value(1), Value(2), Value(3), Value(4));

  EXPECT_EQ(first, "sum");
  EXPECT_EQ(remain, (std::vector<int>{1, 2, 3, 4}));
}

TEST(ArgparseTest, InsufficientArguments) {
  EXPECT_THROW(
      { [[maybe_unused]] auto [v] = quick_parse<int>(); }, SyntaxError)
      << "Insufficient arguments for a non-optional parameter should throw "
         "SyntaxError";
}

TEST(ArgparseTest, TooManyArguments) {
  EXPECT_THROW(
      { [[maybe_unused]] auto [v] = quick_parse<int>(Value(1), Value(1)); },
      SyntaxError);
}

TEST(ArgparseTest, TypeMismatch) {
  EXPECT_THROW(
      { [[maybe_unused]] auto [v] = quick_parse<int>(Value("not an int")); },
      TypeError)
      << "Type mismatch for a non-optional int should throw TypeError";
}

TEST(ArgparseTest, OptionalMismatch) {
  auto [optInt, v1] =
      quick_parse<std::optional<int>, std::string>(Value("not an int"));
  EXPECT_FALSE(optInt.has_value())
      << "When an optional int parameter receives an argument of the wrong "
         "type, it returns nullopt";
}

TEST(ArgparseTest, VectorTypeMismatch) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto [vec] =
            quick_parse<std::vector<int>>(Value(1), Value("bad"), Value(3));
      },
      TypeError)
      << "A vector parameter should throw TypeError if any element fails to "
         "convert";
}

TEST(ArgparseTest, EmptyVector) {
  auto [cmd, vec] = quick_parse<std::string, std::vector<int>>(Value("cmd"));
  EXPECT_EQ(cmd, "cmd");
  EXPECT_TRUE(vec.empty()) << "A vector parameter with no corresponding "
                              "arguments should yield an empty vector";
}

TEST(ArgparseTest, PointerMismatch) {
  EXPECT_THROW(
      { [[maybe_unused]] auto [ptr] = quick_parse<int*>(Value("not an int")); },
      TypeError);
}
