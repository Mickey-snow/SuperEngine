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

#include "srbind/arglist_spec.hpp"

namespace serilang {
class VM;
struct Fiber;
}  // namespace serilang

namespace srbind_test {
using namespace srbind;
using serilang::Fiber;
using serilang::VM;

class ParseSpecTest : public ::testing::Test {
 protected:
  using error_type = srbind::type_error;

  // Test utilities to shorten type spelling
  using V = serilang::Value;
  using VV = std::vector<V>;
  using KW = std::unordered_map<std::string, V>;
};

TEST_F(ParseSpecTest, Parsespec) {
  arglist_spec spec =
      parse_spec_impl(arg("first"), arg("second") = 1, vararg, kwargs);
  EXPECT_EQ(spec.GetDebugString(), "(first,second=1)ak");
}

TEST_F(ParseSpecTest, EmptySpec) {
  auto spec = parse_spec_impl();
  EXPECT_EQ(spec.GetDebugString(), "()");
}

TEST_F(ParseSpecTest, DuplicateNamesThrow) {
  EXPECT_THROW(parse_spec_impl(arg("x"), arg("x")), error_type);
}

TEST_F(ParseSpecTest, NamedAfterVarargThrows) {
  EXPECT_THROW(parse_spec_impl(vararg, arg("x")), error_type);
}

TEST_F(ParseSpecTest, VarargAfterKwargThrows) {
  EXPECT_THROW(parse_spec_impl(kwargs, vararg), error_type);
}

TEST_F(ParseSpecTest, DuplicateVarargThrows) {
  EXPECT_THROW(parse_spec_impl(vararg, vararg), error_type);
}

TEST_F(ParseSpecTest, DuplicateKwargThrows) {
  EXPECT_THROW(parse_spec_impl(kwargs, kwargs), error_type);
}

TEST_F(ParseSpecTest, PosAfterKwonlyThrows) {
  EXPECT_THROW(parse_spec_impl(kw_arg("x"), arg("y")), error_type);
}

TEST_F(ParseSpecTest, TraitParse_NoVarNoKw) {
  auto s = srbind::parse_spec_impl<void(int, double)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0,arg_1)");
}

TEST_F(ParseSpecTest, TraitParse_Vararg) {
  auto s = srbind::parse_spec_impl<void(int, VV)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
}

TEST_F(ParseSpecTest, TraitParse_Kwarg) {
  auto s = srbind::parse_spec_impl<void(int, KW)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)k");
}

TEST_F(ParseSpecTest, TraitParse_VarargKwarg) {
  auto s = srbind::parse_spec_impl<void(int, VV, KW)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)ak");
}

TEST_F(ParseSpecTest, TraitParse_KwargVararg) {
  auto s = srbind::parse_spec_impl<void(KW, VV)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
  // KW counted as positional, VV recognized as vararg
}

TEST_F(ParseSpecTest, TraitParse_OnlyVararg) {
  auto s = srbind::parse_spec_impl<void(VV)>();
  EXPECT_EQ(s.GetDebugString(), "()a");
}

TEST_F(ParseSpecTest, TraitParse_OnlyKwarg) {
  auto s = srbind::parse_spec_impl<void(KW)>();
  EXPECT_EQ(s.GetDebugString(), "()k");
}

TEST_F(ParseSpecTest, TraitParse_MemberFunctionPointer) {
  struct C {
    void m1(int, VV, KW) {}
    void m2(double, KW) const {}
  };

  auto s1 = srbind::parse_spec_impl<decltype(&C::m1)>();
  EXPECT_EQ(s1.GetDebugString(), "(arg_0)ak");

  auto s2 = srbind::parse_spec_impl<decltype(&C::m2)>();
  EXPECT_EQ(s2.GetDebugString(), "(arg_0)k");
}

TEST_F(ParseSpecTest, TraitParse_Functor) {
  struct Functor {
    void operator()(int, VV) const {}
  };

  auto s = srbind::parse_spec_impl<Functor>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
}

TEST_F(ParseSpecTest, TraitParse_HasVmFib) {
  auto s = srbind::parse_spec_impl<void(VM&, Fiber&, int)>();
  EXPECT_EQ(s.GetDebugString(), "vf(arg_0)");
}

TEST_F(ParseSpecTest, TraitParse_Vm) {
  auto s = srbind::parse_spec_impl<void(VM&, int)>();
  EXPECT_EQ(s.GetDebugString(), "v(arg_0)");
}

TEST_F(ParseSpecTest, TraitParse_Fib) {
  auto s = srbind::parse_spec_impl<void(Fiber&, int)>();
  EXPECT_EQ(s.GetDebugString(), "f(arg_0)");
}

TEST_F(ParseSpecTest, ArgumentCountMismatch) {
  // won't compile
  // srbind::parse_spec<void(int)>(arg("first"), arg("extra1"));
  // srbind::parse_spec<void(int, int, int)>(arg("no_second_third"));
  // srbind::parse_spec<void(Fiber&)>(arg("extra1"));
  // srbind::parse_spec<void(Fiber&, int, int)>(arg("no_second"));
}

TEST_F(ParseSpecTest, TraitParse_VmWithNamed) {
  auto s = srbind::parse_spec<void(VM&, int)>(arg("first"));
  EXPECT_EQ(s.GetDebugString(), "v(first)");
}

TEST_F(ParseSpecTest, TraitParse_FibWithNamed) {
  auto s = srbind::parse_spec<void(Fiber&, int, double)>(arg("first"),
                                                         arg("second"));
  EXPECT_EQ(s.GetDebugString(), "f(first,second)");
}

TEST_F(ParseSpecTest, TraitParse_VmFibWithNamed) {
  auto s = srbind::parse_spec<void(VM&, Fiber&, int, double, std::string)>(
      arg("first"), arg("second"), arg("third") = "3");
  EXPECT_EQ(s.GetDebugString(), "vf(first,second,third=3)");
}

}  // namespace srbind_test
