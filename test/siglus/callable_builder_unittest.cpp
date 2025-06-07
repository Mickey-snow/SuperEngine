// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include <boost/container/small_vector.hpp>

#include "libsiglus/callable_builder.hpp"

using namespace libsiglus::elm::callable_builder;
using namespace libsiglus::elm;
using libsiglus::Type;

// compile time checks
static constexpr auto fb_ping = fn("Ping")[0]().ret(Type::None);
static_assert(fb_ping.index == 0);
static_assert(fb_ping.name == "Ping");
static_assert(fb_ping.ret == Type::None);

TEST(CallableDsl, Basic) {
  auto callable =
      make_callable(fn("Get")[0]().ret(Type::None),                    // 0 args
                    fn("Set")[1](Type::Int).ret(Type::None),           // 1 arg
                    fn("Add")[2](Type::Int, Type::Int).ret(Type::Int)  // 2 args
      );

  {
    const Function& f = callable.overloads.at(0);
    EXPECT_EQ(f.name, "Get");
    EXPECT_TRUE(f.arg_t.empty());
    EXPECT_EQ(f.return_t, Type::None);
  }

  {
    const Function& f = callable.overloads.at(1);
    EXPECT_EQ(f.name, "Set");
    ASSERT_EQ(f.arg_t.size(), 1u);
    EXPECT_EQ(f.arg_t[0], Type::Int);
    EXPECT_EQ(f.return_t, Type::None);
  }

  {
    const Function& f = callable.overloads.at(2);
    EXPECT_EQ(f.name, "Add");
    ASSERT_EQ(f.arg_t.size(), 2u);
    EXPECT_EQ(f.arg_t[0], Type::Int);
    EXPECT_EQ(f.arg_t[1], Type::Int);
    EXPECT_EQ(f.return_t, Type::Int);
  }
}

TEST(CallableDsl, DebugString) {
  auto echo = fn("Echo")[7](Type::Int).ret(Type::Int);
  auto cat = fn("Cat")[1]().ret(Type::None);
  auto callable = make_callable(echo, cat);

  EXPECT_EQ(callable.ToDebugString(),
            ".<callable Echo[7](int)->int  Cat[1]()->null_t>");
}

TEST(CallableDsl, AnyOverload) {
  // fn foo(int, int=1) => overload[1](int), overload[any](int,int)
  auto foo = make_callable(fn("foo")[1](Type::Int).ret(Type::Int),
                           fn("foo")[any](Type::Int, Type::Int).ret(Type::Int));
  EXPECT_EQ(foo.ToDebugString(),
            ".<callable foo[1](int)->int  foo[](int,int)->int>");
}

TEST(CallableDsl, Vararg) {
  // fn foo(int, string...)
  auto foo = make_callable(
      fn("foo")[any](Type::Int, va_arg(Type::String)).ret(Type::None));
  EXPECT_EQ(foo.ToDebugString(), ".<callable foo[](int,str...)->null_t>");
}
