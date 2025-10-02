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

namespace srbind_test {

using namespace srbind;

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
      parse_spec(arg("first"), arg("second") = 1, vararg, kwargs);
  EXPECT_EQ(spec.GetDebugString(), "(first,second=1)ak");
}

TEST_F(ParseSpecTest, EmptySpec) {
  auto spec = parse_spec();
  EXPECT_EQ(spec.GetDebugString(), "()");
}

TEST_F(ParseSpecTest, DuplicateNamesThrow) {
  EXPECT_THROW(parse_spec(arg("x"), arg("x")), error_type);
}

TEST_F(ParseSpecTest, NamedAfterVarargThrows) {
  EXPECT_THROW(parse_spec(vararg, arg("x")), error_type);
}

TEST_F(ParseSpecTest, VarargAfterKwargThrows) {
  EXPECT_THROW(parse_spec(kwargs, vararg), error_type);
}

TEST_F(ParseSpecTest, DuplicateVarargThrows) {
  EXPECT_THROW(parse_spec(vararg, vararg), error_type);
}

TEST_F(ParseSpecTest, DuplicateKwargThrows) {
  EXPECT_THROW(parse_spec(kwargs, kwargs), error_type);
}

TEST_F(ParseSpecTest, PosAfterKwonlyThrows) {
  EXPECT_THROW(parse_spec(kw_arg("x"), arg("y")), error_type);
}

TEST_F(ParseSpecTest, TraitParse_NoVarNoKw) {
  auto s = srbind::parse_spec<void(int, double)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0,arg_1)");
}

TEST_F(ParseSpecTest, TraitParse_Vararg) {
  auto s = srbind::parse_spec<void(int, VV)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
}

TEST_F(ParseSpecTest, TraitParse_Kwarg) {
  auto s = srbind::parse_spec<void(int, KW)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)k");
}

TEST_F(ParseSpecTest, TraitParse_VarargKwarg) {
  auto s = srbind::parse_spec<void(int, VV, KW)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)ak");
}

TEST_F(ParseSpecTest, TraitParse_KwargVararg) {
  auto s = srbind::parse_spec<void(KW, VV)>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
  // KW counted as positional, VV recognized as vararg
}

TEST_F(ParseSpecTest, TraitParse_OnlyVararg) {
  auto s = srbind::parse_spec<void(VV)>();
  EXPECT_EQ(s.GetDebugString(), "()a");
}

TEST_F(ParseSpecTest, TraitParse_OnlyKwarg) {
  auto s = srbind::parse_spec<void(KW)>();
  EXPECT_EQ(s.GetDebugString(), "()k");
}

TEST_F(ParseSpecTest, TraitParse_MemberFunctionPointer) {
  struct C {
    void m1(int, VV, KW) {}
    void m2(double, KW) const {}
  };

  auto s1 = srbind::parse_spec<decltype(&C::m1)>();
  EXPECT_EQ(s1.GetDebugString(), "(arg_0)ak");

  auto s2 = srbind::parse_spec<decltype(&C::m2)>();
  EXPECT_EQ(s2.GetDebugString(), "(arg_0)k");
}

TEST_F(ParseSpecTest, TraitParse_Functor) {
  struct Functor {
    void operator()(int, VV) const {}
  };

  auto s = srbind::parse_spec<Functor>();
  EXPECT_EQ(s.GetDebugString(), "(arg_0)a");
}

}  // namespace srbind_test
