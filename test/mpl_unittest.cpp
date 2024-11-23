// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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

#include "utilities/mpl.hpp"

#include <type_traits>

TEST(TypelistTest, SizeOfTypeList) {
  using EmptyList = TypeList<>;
  using SingleList = TypeList<int>;
  using MultipleList = TypeList<int, char, double>;

  EXPECT_EQ(SizeOfTypeList<EmptyList>::value, 0);
  EXPECT_EQ(SizeOfTypeList<SingleList>::value, 1);
  EXPECT_EQ(SizeOfTypeList<MultipleList>::value, 3);
}

TEST(TypelistTest, AddFront) {
  using OriginalList = TypeList<int, char, double>;
  using ExpectedList = TypeList<float, int, char, double>;
  using ResultList = typename AddFront<float, OriginalList>::type;

  EXPECT_TRUE((std::is_same<ResultList, ExpectedList>::value));
}

TEST(TypelistTest, AddBack) {
  using OriginalList = TypeList<int, char, double>;
  using ExpectedList = TypeList<int, char, double, float>;
  using ResultList = typename AddBack<OriginalList, float>::type;

  EXPECT_TRUE((std::is_same<ResultList, ExpectedList>::value));
}

TEST(TypelistTest, GetNthType) {
  using MyList = TypeList<int, char, double, float>;

  using FirstType = typename GetNthType<0, MyList>::type;
  using SecondType = typename GetNthType<1, MyList>::type;
  using ThirdType = typename GetNthType<2, MyList>::type;
  using FourthType = typename GetNthType<3, MyList>::type;

  EXPECT_TRUE((std::is_same<FirstType, int>::value));
  EXPECT_TRUE((std::is_same<SecondType, char>::value));
  EXPECT_TRUE((std::is_same<ThirdType, double>::value));
  EXPECT_TRUE((std::is_same<FourthType, float>::value));
}

TEST(TypelistTest, Append) {
  using List1 = TypeList<int, char>;
  using List2 = TypeList<double, float>;
  using ExpectedList = TypeList<int, char, double, float>;
  using ResultList = typename Append<List1, List2>::type;

  EXPECT_TRUE((std::is_same<ResultList, ExpectedList>::value));
}

TEST(TypelistTest, AddToEmptyList) {
  using EmptyList = TypeList<>;
  using ExpectedListFront = TypeList<int>;
  using ExpectedListBack = TypeList<int>;
  using ResultListFront = typename AddFront<int, EmptyList>::type;
  using ResultListBack = typename AddBack<EmptyList, int>::type;

  EXPECT_TRUE((std::is_same<ResultListFront, ExpectedListFront>::value));
  EXPECT_TRUE((std::is_same<ResultListBack, ExpectedListBack>::value));
}

TEST(TypelistTest, AppendEmptyLists) {
  using EmptyList = TypeList<>;
  using List = TypeList<int, char>;
  using ExpectedList = TypeList<int, char>;

  using ResultList1 = typename Append<EmptyList, List>::type;
  using ResultList2 = typename Append<List, EmptyList>::type;
  using ResultList3 = typename Append<EmptyList, EmptyList>::type;

  EXPECT_TRUE((std::is_same<ResultList1, ExpectedList>::value));
  EXPECT_TRUE((std::is_same<ResultList2, ExpectedList>::value));
  EXPECT_TRUE((std::is_same<ResultList3, EmptyList>::value));
}

// Test GetNthType with index out of bounds (should trigger static_assert)
// This test will fail to compile if uncommented, as intended
/*
TEST(TypelistTest, GetNthTypeOutOfBounds) {
    using MyList = TypeList<int, char, double>;
    using InvalidType = typename GetNthType<3, MyList>::type; // Index out of
bounds
}
*/

TEST(TypelistTest, ComplexAppend) {
  using List1 = TypeList<int, char>;
  using List2 = TypeList<double, float>;
  using List3 = TypeList<long, short>;
  using ExpectedList = TypeList<int, char, double, float, long, short>;
  using ResultList =
      typename Append<typename Append<List1, List2>::type, List3>::type;

  EXPECT_TRUE((std::is_same<ResultList, ExpectedList>::value));
}

TEST(TypelistTest, NestedTypeLists) {
  using InnerList = TypeList<float, double>;
  using OuterList = TypeList<int, char, InnerList>;
  using ExpectedList = TypeList<int, char, TypeList<float, double>>;

  EXPECT_TRUE((std::is_same<OuterList, ExpectedList>::value));
}

TEST(TypelistTest, SizeWithNestedTypeLists) {
  using InnerList = TypeList<float, double>;
  using OuterList = TypeList<int, char, InnerList>;

  EXPECT_EQ(SizeOfTypeList<OuterList>::value, 3);
}
