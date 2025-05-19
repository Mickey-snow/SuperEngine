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

#include "vm/gc.hpp"
#include "vm/value.hpp"
#include "vm/value_internal/dict.hpp"
#include "vm/value_internal/list.hpp"

namespace value_test {
using namespace serilang;

TEST(ValueBasic, TruthinessAndType) {
  Value nil;
  Value bTrue(true);
  Value bFalse(false);
  Value iZero(0);
  Value iPos(42);
  Value dZero(0.0);
  Value dPos(3.14);
  Value strEmpty(std::string{});
  Value strNonEmpty(std::string{"hi"});

  EXPECT_FALSE(nil.IsTruthy());
  EXPECT_TRUE(bTrue.IsTruthy());
  EXPECT_FALSE(bFalse.IsTruthy());
  EXPECT_FALSE(iZero.IsTruthy());
  EXPECT_TRUE(iPos.IsTruthy());
  EXPECT_FALSE(dZero.IsTruthy());
  EXPECT_TRUE(dPos.IsTruthy());
  EXPECT_FALSE(strEmpty.IsTruthy());
  EXPECT_TRUE(strNonEmpty.IsTruthy());

  EXPECT_EQ(nil.Type(), ObjType::Nil);
  EXPECT_EQ(bTrue.Type(), ObjType::Bool);
  EXPECT_EQ(iPos.Type(), ObjType::Int);
  EXPECT_EQ(dPos.Type(), ObjType::Double);
  EXPECT_EQ(strNonEmpty.Type(), ObjType::Str);
}

TEST(ValueNumeric, IntAndDoubleArithmetic) {
  Value a(6), b(3), c(2.5), d(1.5);

  EXPECT_TRUE(a.Operator(Op::Add, b) == 9);
  EXPECT_TRUE(a.Operator(Op::Sub, b) == 3);
  EXPECT_TRUE(a.Operator(Op::Mul, b) == 18);
  EXPECT_TRUE(a.Operator(Op::Div, b) == 2);

  EXPECT_TRUE(c.Operator(Op::Add, d) == 4.0);
  EXPECT_TRUE(c.Operator(Op::Sub, d) == 1.0);
  EXPECT_TRUE(c.Operator(Op::Mul, d) == 3.75);
  EXPECT_TRUE(c.Operator(Op::Div, d) == (2.5 / 1.5));

  // mixed int / double
  EXPECT_TRUE(a.Operator(Op::Add, d) == 7.5);
  EXPECT_TRUE(c.Operator(Op::Mul, b) == 7.5);
}

TEST(ValueNumeric, Comparisons) {
  Value one(1), two(2), oneD(1.0), twoD(2.0);

  EXPECT_TRUE(one.Operator(Op::Less, two) == true);
  EXPECT_TRUE(one.Operator(Op::GreaterEqual, one) == true);
  EXPECT_TRUE(twoD.Operator(Op::Equal, two) == true);
  EXPECT_TRUE(oneD.Operator(Op::NotEqual, twoD) == true);
}

TEST(ValueNumeric, UnaryOperators) {
  Value five(5), minusFive(-5), pi(3.14);

  EXPECT_TRUE(five.Operator(Op::Sub) == -5);
  EXPECT_TRUE(minusFive.Operator(Op::Sub) == 5);
  EXPECT_TRUE(pi.Operator(Op::Sub) == -3.14);
}

TEST(ValueInt, BitwiseShift) {
  Value v1(1), shift3(3);

  EXPECT_TRUE(v1.Operator(Op::ShiftLeft, shift3) == 8);
  EXPECT_TRUE(Value(15).Operator(Op::BitAnd, Value(9)) == 9);
  EXPECT_TRUE(Value(12).Operator(Op::BitOr, Value(3)) == 15);
  EXPECT_TRUE(Value(5).Operator(Op::BitXor, Value(1)) == 4);
}

TEST(ValueBool, LogicalOps) {
  Value t(true), f(false);

  EXPECT_TRUE(t.Operator(Op::LogicalAnd, f) == false);
  EXPECT_TRUE(t.Operator(Op::LogicalOr, f) == true);
  EXPECT_TRUE(f.Operator(Op::LogicalOr, f) == false);

  // unary logical NOT (mapped to Op::Tilde)
  EXPECT_TRUE(t.Operator(Op::Tilde) == false);
  EXPECT_TRUE(f.Operator(Op::Tilde) == true);
}

TEST(ValueString, ConcatenateAndRepeat) {
  Value hello(std::string{"hello"});
  Value world(std::string{"world"});
  Value three(3);

  EXPECT_TRUE(hello.Operator(Op::Add, world) == std::string("helloworld"));
  EXPECT_TRUE(Value(std::string{"ab"}).Operator(Op::Mul, three) ==
              std::string("ababab"));
}

TEST(ValueEdge, DivisionByZero) {
  Value six(6), zero(0);

  // int / 0 returns 0 per implementation
  EXPECT_TRUE(six.Operator(Op::Div, zero) == 0);

  // double / 0.0 returns 0.0 per implementation
  EXPECT_TRUE(Value(4.2).Operator(Op::Div, Value(0.0)) == 0.0);
}

TEST(ValueContainer, ListAndDict) {
  GarbageCollector gc;

  // empty & filled lists
  Value lstEmpty(gc.Allocate<List>());
  Value lstFilled(
      gc.Allocate<List>(std::vector<Value>{Value(1), Value(2), Value(3)}));

  // empty & filled dicts
  Value dictEmpty(gc.Allocate<Dict>());
  Value dictFilled(gc.Allocate<Dict>(
      std::unordered_map<std::string, Value>{{"a", Value(1)}}));

  // All container objects are truthy
  EXPECT_TRUE(lstEmpty.IsTruthy());
  EXPECT_TRUE(lstFilled.IsTruthy());
  EXPECT_TRUE(dictEmpty.IsTruthy());
  EXPECT_TRUE(dictFilled.IsTruthy());

  // Correct dynamic type
  EXPECT_EQ(lstEmpty.Type(), ObjType::List);
  EXPECT_EQ(lstFilled.Type(), ObjType::List);
  EXPECT_EQ(dictEmpty.Type(), ObjType::Dict);
  EXPECT_EQ(dictFilled.Type(), ObjType::Dict);

  // Simple Desc() sanity checks
  EXPECT_EQ(lstEmpty.Desc(), "<list[0]>");
  EXPECT_EQ(lstFilled.Desc(), "<list[3]>");
  EXPECT_EQ(dictEmpty.Desc(), "<dict{0}>");
  EXPECT_EQ(dictFilled.Desc(), "<dict{1}>");

  // Str() formatting smoke test
  EXPECT_EQ(lstFilled.Str(), "[1,2,3]");
  EXPECT_EQ(lstEmpty.Str(), "[]");
  EXPECT_EQ(dictFilled.Str(), "{a:1}");
  EXPECT_EQ(dictEmpty.Str(), "{}");
}

}  // namespace value_test
