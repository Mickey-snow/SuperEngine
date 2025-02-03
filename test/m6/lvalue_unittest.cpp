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

#include "m6/op.hpp"
#include "m6/symbol_table.hpp"
#include "m6/value.hpp"
#include "m6/value_internal/lvalue.hpp"

#include "util.hpp"

using namespace m6;

class lValueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Value dummy = make_value(123);

    sym_tab = std::make_shared<SymbolTable>();
    sym_tab->Set("value", dummy);

    lval = std::make_shared<lValue>(sym_tab, "value");
  }

  std::shared_ptr<SymbolTable> sym_tab;
  Value lval;
};

TEST_F(lValueTest, Basic) {
  EXPECT_EQ(lval->Str(), "123");
  EXPECT_EQ(lval->Desc(), "<int: 123>");
  EXPECT_EQ(lval->Type(), typeid(int));
  EXPECT_NO_THROW({ EXPECT_EQ(std::any_cast<int>(lval->Get()), 123); });
}

TEST_F(lValueTest, ProxyOperators) {
  Value result = nullptr;
  EXPECT_NO_THROW({
    result = lval->Operator(Op::Add, make_value(100));
    EXPECT_VALUE_EQ(result, 100 + 123);
  });

  EXPECT_NO_THROW({
    result = lval->Operator(Op::Sub);
    EXPECT_VALUE_EQ(result, -123);
  });
}

// Assignment operator
TEST_F(lValueTest, Declare) {
  {
    Value lval = std::make_shared<lValue>(sym_tab, "v2");
    Value ret = lval->Operator(Op::Assign, make_value(89));
    EXPECT_VALUE_EQ(ret, 89);
    EXPECT_NO_THROW({
      EXPECT_TRUE(sym_tab->Exists("v2"));
      EXPECT_VALUE_EQ(sym_tab->Get("v2"), 89);
    });
  }

  {
    Value lval = std::make_shared<lValue>(sym_tab, "v3");
    Value ret = lval->Operator(Op::Assign, make_value("hello"));
    EXPECT_VALUE_EQ(ret, "hello");
    EXPECT_NO_THROW({
      EXPECT_TRUE(sym_tab->Exists("v3"));
      EXPECT_VALUE_EQ(sym_tab->Get("v3"), "hello");
    });
  }

  {
    Value lval = std::make_shared<lValue>(sym_tab, "v4");
    Value ret =
        lval->Operator(Op::Assign, std::make_shared<lValue>(sym_tab, "value"));

    sym_tab->Remove("value");

    EXPECT_VALUE_EQ(ret, 123);
    EXPECT_NO_THROW({
      EXPECT_TRUE(sym_tab->Exists("v4"));
      EXPECT_VALUE_EQ(sym_tab->Get("v4"), 123);
    });
  }
}

// Compound assignment operators
TEST_F(lValueTest, CompoundAddAssign) {
  Value ret = lval->Operator(Op::AddAssign, make_value(100));
  EXPECT_VALUE_EQ(ret, 223);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 223);
}

TEST_F(lValueTest, CompoundSubAssign) {
  Value ret = lval->Operator(Op::SubAssign, make_value(50));
  EXPECT_VALUE_EQ(ret, 73);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 73);
}

TEST_F(lValueTest, CompoundMulAssign) {
  Value ret = lval->Operator(Op::MulAssign, make_value(2));
  EXPECT_VALUE_EQ(ret, 246);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 246);
}

TEST_F(lValueTest, CompoundDivAssign) {
  Value ret = lval->Operator(Op::DivAssign, make_value(3));
  EXPECT_VALUE_EQ(ret, 41);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 41);
}

TEST_F(lValueTest, CompoundModAssign) {
  Value ret = lval->Operator(Op::ModAssign, make_value(100));
  EXPECT_VALUE_EQ(ret, 23);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 23);
}

TEST_F(lValueTest, CompoundBitAndAssign) {
  Value ret = lval->Operator(Op::BitAndAssign, make_value(100));
  EXPECT_VALUE_EQ(ret, 96);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 96);
}

TEST_F(lValueTest, CompoundBitOrAssign) {
  Value ret = lval->Operator(Op::BitOrAssign, make_value(100));
  EXPECT_VALUE_EQ(ret, 127);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 127);
}

TEST_F(lValueTest, CompoundBitXorAssign) {
  Value ret = lval->Operator(Op::BitXorAssign, make_value(100));
  EXPECT_VALUE_EQ(ret, 31);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 31);
}

TEST_F(lValueTest, CompoundShiftLeftAssign) {
  Value ret = lval->Operator(Op::ShiftLeftAssign, make_value(2));
  EXPECT_VALUE_EQ(ret, 492);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 492);
}

TEST_F(lValueTest, CompoundShiftRightAssign) {
  Value ret = lval->Operator(Op::ShiftRightAssign, make_value(2));
  EXPECT_VALUE_EQ(ret, 30);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 30);
}

TEST_F(lValueTest, CompoundShiftUnsignedRightAssign) {
  Value ret = lval->Operator(Op::ShiftUnsignedRightAssign, make_value(2));
  EXPECT_VALUE_EQ(ret, 30);
  EXPECT_VALUE_EQ(sym_tab->Get("value"), 30);
}
