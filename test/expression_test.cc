// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "gtest/gtest.h"

#include "libreallive/archive.h"
#include "libreallive/expression.h"
#include "libreallive/intmemref.h"
#include "machine/rlmachine.h"
#include "modules/module_jmp.h"
#include "test_system/test_system.h"

#include "test_utils.h"

using namespace std;
using namespace libreallive;

TEST(ExpressionTest, BasicArithmatic) {
  TestSystem system;
  libreallive::Archive arc(
      LocateTestCase("ExpressionTest_SEEN/basicOperators.TXT"));
  RLMachine rlmachine(system, arc);
  rlmachine.ExecuteUntilHalted();

  int values[10];
  for (int i = 0; i < 10; ++i)
    values[i] = rlmachine.GetIntValue(IntMemRef('A', i));

  EXPECT_EQ(2, values[0]) << "Incorect value for intA[0] (+ test)";
  EXPECT_EQ(3, values[1]) << "Incorect value for intA[1] (+= test)";
  EXPECT_EQ(0, values[2]) << "Incorect value for intA[2] (- test)";
  EXPECT_EQ(3, values[3]) << "Incorect value for intA[3] (-= test)";
  EXPECT_EQ(10, values[4]) << "Incorect value for intA[4] (* test)";
  EXPECT_EQ(30, values[5]) << "Incorect value for intA[5] (*= test)";
  EXPECT_EQ(10, values[6]) << "Incorect value for intA[6] (/ test)";
  EXPECT_EQ(2, values[7]) << "Incorect value for intA[7] (/= test)";
  EXPECT_EQ(2, values[8]) << "Incorect value for intA[8] (% test)";
  EXPECT_EQ(1, values[9]) << "Incorect value for intA[9] (%= test)";
}

TEST(ExpressionTest, ComparisonOperators) {
  TestSystem system;
  libreallive::Archive arc(
      LocateTestCase("ExpressionTest_SEEN/comparisonOperators.TXT"));
  RLMachine rlmachine(system, arc);
  rlmachine.ExecuteUntilHalted();

  int values[14];
  for (int i = 0; i < 14; ++i)
    values[i] = rlmachine.GetIntValue(IntMemRef('A', i));

  EXPECT_EQ(0, values[0]) << "Incorect value for intA[0]";
  EXPECT_EQ(1, values[1]) << "Incorect value for intA[1]";
  EXPECT_EQ(0, values[2]) << "Incorect value for intA[2]";
  EXPECT_EQ(1, values[3]) << "Incorect value for intA[3]";
  EXPECT_EQ(1, values[4]) << "Incorect value for intA[4]";
  EXPECT_EQ(1, values[5]) << "Incorect value for intA[5]";
  EXPECT_EQ(0, values[6]) << "Incorect value for intA[6]";
  EXPECT_EQ(1, values[7]) << "Incorect value for intA[7]";
  EXPECT_EQ(0, values[8]) << "Incorect value for intA[8]";
  EXPECT_EQ(1, values[9]) << "Incorect value for intA[9]";
  EXPECT_EQ(1, values[10]) << "Incorect value for intA[10]";
  EXPECT_EQ(0, values[11]) << "Incorect value for intA[11]";
  EXPECT_EQ(1, values[12]) << "Incorect value for intA[12]";
  EXPECT_EQ(0, values[13]) << "Incorect value for intA[13]";
}

TEST(ExpressionTest, LogicalOperators) {
  TestSystem system;
  libreallive::Archive arc(
      LocateTestCase("ExpressionTest_SEEN/logicalOperators.TXT"));
  RLMachine rlmachine(system, arc);
  rlmachine.ExecuteUntilHalted();

  int values[7];
  for (int i = 0; i < 7; ++i)
    values[i] = rlmachine.GetIntValue(IntMemRef('A', i));

  EXPECT_EQ(1, values[0]) << "Incorect value for intA[0]";
  EXPECT_EQ(0, values[1]) << "Incorect value for intA[1]";
  EXPECT_EQ(1, values[2]) << "Incorect value for intA[2]";
  EXPECT_EQ(1, values[3]) << "Incorect value for intA[3]";
  EXPECT_EQ(1, values[4]) << "Incorect value for intA[4]";
  EXPECT_EQ(0, values[5]) << "Incorect value for intA[5]";
  EXPECT_EQ(0, values[6]) << "Incorect value for intA[6]";
}

TEST(ExpressionTest, PreviousErrors) {
  TestSystem system;
  libreallive::Archive arc(
      LocateTestCase("ExpressionTest_SEEN/previousErrors.TXT"));
  RLMachine rlmachine(system, arc);
  rlmachine.GetModuleManager().AttachModule(new JmpModule);
  rlmachine.ExecuteUntilHalted();

  int values[6];
  for (int i = 0; i < 6; ++i)
    values[i] = rlmachine.GetIntValue(IntMemRef('B', i));

  EXPECT_EQ(1, values[0]) << "Incorect value for intB[0]";
  EXPECT_EQ(1, values[1]) << "Incorect value for intB[1]";
  EXPECT_EQ(1, values[2]) << "Incorect value for intB[2]";
  EXPECT_EQ(0, values[3]) << "Incorect value for intB[3]";
  EXPECT_EQ(0, values[4]) << "Incorect value for intB[4]";
  EXPECT_EQ(10, values[5]) << "Incorect value for intB[5]";
}
