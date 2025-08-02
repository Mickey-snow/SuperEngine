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

#include "srbind/argloader.hpp"
#include "vm/value.hpp"

#include <utility>
#include <vector>

namespace srbind_test {

using namespace srbind;
using namespace serilang;

template <typename T>
concept value_type =
    Contains<T,
             TypeList<std::monostate, bool, int, double, std::string>>::value;

class LoadArgsTest : public ::testing::Test {
 protected:
  template <typename... Ts, typename... Us>
    requires(value_type<Ts> && ...)
  inline static std::vector<Value> Stack(Ts&&... params) {
    // layout: (arg0, arg1,..., kw0, kwarg0, kw1, kwarg1,...)
    return std::vector<Value>{Value(std::forward<Ts>(params))...};
  }

  inline static arglist_spec make_spec(std::size_t nparam,
                                       bool has_vararg = false,
                                       bool has_kwarg = false) {
    arglist_spec s;
    s.nparam = nparam;
    s.has_vararg = has_vararg;
    s.has_kwarg = has_kwarg;
    return s;
  }

  using KW = std::unordered_map<std::string, Value>;
  using V = std::vector<Value>;

  using error_type = srbind::argloader_error;
};

TEST_F(LoadArgsTest, Positional) {
  auto spec = make_spec(3);  // 3 positional, no vararg/kwarg
  std::vector<serilang::Value> stack = Stack(1, 2.5, std::string("hello"));
  auto [i, d, s] = load_args<int, double, std::string>(stack, 3, 0, spec);
  EXPECT_EQ(i, 1);
  EXPECT_DOUBLE_EQ(d, 2.5);
  EXPECT_EQ(s, "hello");
  EXPECT_TRUE(stack.empty());
}

TEST_F(LoadArgsTest, KeywordPairs) {
  auto spec = make_spec(2, /*vararg=*/false, /*kwarg=*/true);
  // Positional: 10, 20. Keyword: ("kw", 42)
  std::vector<serilang::Value> stack = Stack(10, 20, std::string("kw"), 42);
  auto [a, b, kw] = load_args<int, int, KW>(stack, 2, 1, spec);
  EXPECT_EQ(a, 10);
  EXPECT_EQ(b, 20);
  EXPECT_EQ(kw.at("kw"), 42);
  EXPECT_TRUE(stack.empty());  // both positional and keyword consumed
}

TEST_F(LoadArgsTest, Vararg) {
  auto spec = make_spec(1, /*vararg=*/true, /*kwarg=*/false);
  // Fixed positional: 8. Varargs (leftover) 9, 7
  std::vector<serilang::Value> stack = Stack(8, 9, 7);
  // Here we simulate varargs being deeper in stack; only fixed positional is
  // last
  auto [x, v] = load_args<int, V>(stack, 3, 0, spec);
  EXPECT_EQ(x, 8);
  ASSERT_EQ(v.size(), 2);
  EXPECT_EQ(v.at(0), 9);
  EXPECT_EQ(v.at(1), 7);
}

TEST_F(LoadArgsTest, VarargAndKwarg) {
  auto spec = make_spec(1, /*vararg=*/true, /*kwarg=*/true);
  // Layout: fixed positional 42, [vararg items 100,200], keyword ("k", 5)
  std::vector<serilang::Value> stack = Stack(42, 100, 200, std::string("k"), 5);
  auto [fixed, v, kw] = load_args<double, V, KW>(stack, 3, 1, spec);
  EXPECT_DOUBLE_EQ(fixed, 42.0);
  ASSERT_EQ(v.size(), 2);
  EXPECT_EQ(v.at(0), 100);
  EXPECT_EQ(v.at(1), 200);
  ASSERT_TRUE(kw.contains("k"));
  EXPECT_EQ(kw.at("k"), 5);
}

TEST_F(LoadArgsTest, DefaultArg) {
  auto spec = make_spec(1, /*vararg=*/false, /*kwarg=*/true);
  spec.param_index["k"] = 0;
  spec.defaults[0] = []() { return TempValue(Value("default")); };

  std::vector<serilang::Value> stack = Stack(std::string("m"), 1);
  auto [k, kw] = load_args<std::string, KW>(stack, 0, 1, spec);
  EXPECT_EQ(k, "default");
  ASSERT_TRUE(kw.contains("m"));
  EXPECT_EQ(kw.at("m"), 1);
}

TEST_F(LoadArgsTest, MissingArguments) {
  {
    auto spec = make_spec(2);
    std::vector<serilang::Value> stack = Stack(1);
    EXPECT_THROW((load_args<int, int>(stack, 1, 0, spec)), error_type);
  }

  {
    auto spec = make_spec(3);  // expects 3 positional
    std::vector<serilang::Value> stack = Stack(1, 2);
    EXPECT_THROW((load_args<int, int>(stack, 2, 0, spec)), error_type);
  }
}

TEST_F(LoadArgsTest, TooManyArguments) {
  {
    auto spec = make_spec(2);
    std::vector<serilang::Value> stack = Stack(1, 2, 3);
    EXPECT_THROW((load_args<int, int>(stack, 3, 0, spec)), error_type);
  }

  {
    auto spec = make_spec(3);
    std::vector<serilang::Value> stack = Stack(1, 2, std::string("k"), 3);
    EXPECT_THROW((load_args<int, int>(stack, 2, 1, spec)), error_type);
  }
}

TEST_F(LoadArgsTest, StackUnderflow) {
  auto spec = make_spec(1);
  std::vector<serilang::Value> stack = Stack();
  EXPECT_THROW((load_args<int>(stack, 0, 0, spec)), error_type);
}

TEST_F(LoadArgsTest, MultipleAssign) {
  auto spec = make_spec(1);
  spec.param_index["k"] = 0;
  std::vector<serilang::Value> stack = Stack(1, std::string("k"), 1);
  EXPECT_THROW((load_args<int>(stack, 1, 1, spec)), error_type);
}

TEST_F(LoadArgsTest, DuplicatedKW) {
  auto spec = make_spec(0, /*vararg=*/false, /*kwarg=*/true);
  std::vector<serilang::Value> stack =
      Stack(std::string("k"), 1, std::string("k"), 2);
  EXPECT_THROW((load_args<KW>(stack, 0, 2, spec)), error_type);
}

TEST_F(LoadArgsTest, SinkMissing) {
  auto spec = make_spec(0, true, true);
  std::vector<serilang::Value> stack = Stack(1, std::string("k"), 1);
  EXPECT_THROW((load_args<KW>(stack, 1, 1, spec)), error_type);
  EXPECT_THROW((load_args<V>(stack, 1, 1, spec)), error_type);
  EXPECT_THROW((load_args<>(stack, 1, 1, spec)), error_type);
}

}  // namespace srbind_test
