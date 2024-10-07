// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/tracking.hpp>

#include "test_utils.h"
#include "utilities/lazy_array.h"

#include <algorithm>
#include <iostream>

const int SIZE = 10;

class LazyArrayTest : public ::testing::Test {
 public:
  // Helper class because boost::serialization doesn't like the
  // concept of serializing an int*
  class IntWrapper {
   private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar & num;
    }

    int num;

   public:
    IntWrapper() : num(0) {}
    IntWrapper(int inum) : num(inum) {}

    IntWrapper& operator=(int in) {
      num = in;
      return *this;
    }

    operator int() const { return num; }
  };

 protected:
  template <typename T>
  void populateIntArray(T& lazyArray) {
    // Set each even slot to its number
    for (int i = 0; i < SIZE; ++i)
      if (i % 2 == 0)
        lazyArray[i] = i;
  }

  template <typename T>
  void checkArray(LazyArray<T>& in) {
    // iterating across everything; every other cell should report
    // being unallocated
    FullLazyArrayIterator<T> it = in.fbegin();
    FullLazyArrayIterator<T> end = in.fend();
    for (int i = 0; it != end && i < SIZE; ++it, ++i) {
      if (it.pos() % 2 == 0) {
        EXPECT_TRUE(it.valid()) << "Invalid in valid position";
        EXPECT_EQ(i, it.pos()) << "Pointing to correct item";
        EXPECT_EQ(i, *it) << "Correct value";
      } else {
        EXPECT_FALSE(it.valid()) << "Valid in invalid position";
      }
    }
  }
};

// -----------------------------------------------------------------------

BOOST_CLASS_EXPORT(LazyArrayTest::IntWrapper);
BOOST_CLASS_TRACKING(LazyArrayTest::IntWrapper,
                     boost::serialization::track_always);

// -----------------------------------------------------------------------

TEST_F(LazyArrayTest, EmptyArray) {
  // Empty lazy arrays should simply set size
  LazyArray<int> lazyArray(SIZE);
  EXPECT_EQ(SIZE, lazyArray.Size()) << "Lazy Array didn't remember its size";

  EXPECT_TRUE(lazyArray.begin() == lazyArray.end())
      << "Allocated Lazy iterator is valid on an empty array";

  // iterating across everything; each cell should report being unallocated
  FullLazyArrayIterator<int> it = lazyArray.fbegin();
  FullLazyArrayIterator<int> end = lazyArray.fend();
  for (; it != end; ++it) {
    EXPECT_FALSE(it.valid())
        << "LazyArray with no items says something is valid";
  }
}

TEST_F(LazyArrayTest, AllozatedLazyArrayIterator) {
  LazyArray<int> lazyArray(SIZE);
  EXPECT_EQ(SIZE, lazyArray.Size()) << "Lazy Array didn't remember its size";
  populateIntArray(lazyArray);
  checkArray(lazyArray);

  // Test to make sure that when we use AllocatedLazyArrayIterator, we
  // only stop on items that are valid.
  AllocatedLazyArrayIterator<int> ait = lazyArray.begin();
  AllocatedLazyArrayIterator<int> aend = lazyArray.end();
  EXPECT_TRUE(ait != aend)
      << "Allocated Lazy iterator is invalid on an array with items in it";

  for (int i = 0; ait != aend && i < SIZE; ++ait, i += 2) {
    EXPECT_EQ(0, ait.pos() % 2) << "Stopped on invalid item!";
    EXPECT_EQ(i, *ait) << "Incorrect value " << *ait << " at position " << i;
  }
  checkArray(lazyArray);
}

TEST_F(LazyArrayTest, BothIterators) {
  // Allocate even objects and then these
  LazyArray<int> lazyArray(SIZE);
  EXPECT_EQ(SIZE, lazyArray.Size()) << "Lazy Array didn't remember its size";

  // Go through each item in the array (by FullLazyArrayIterator) and
  // set the values.
  FullLazyArrayIterator<int> it = lazyArray.fbegin();
  FullLazyArrayIterator<int> end = lazyArray.fend();
  for (int i = 0; it != end && i < SIZE; ++it, ++i) {
    EXPECT_FALSE(it.valid())
        << "Position " << i << " should starts off invalid.";
    *it = i;
    EXPECT_TRUE(it.valid())
        << "Position " << i << " should be valid after writing.";
  }

  // Now we should be able to iterate across all the items with
  // AllocatedLazyArrayIterator, and make sure their values are correct.
  AllocatedLazyArrayIterator<int> ait = lazyArray.begin();
  AllocatedLazyArrayIterator<int> aend = lazyArray.end();
  for (int i = 0; ait != aend && i < SIZE; ++ait, ++i) {
    EXPECT_EQ(i, *ait) << "Incorrect value " << *ait << " at position " << i;
  }
}

TEST_F(LazyArrayTest, CopyAssign) {
  LazyArray<int> array(10);
  array[0] = 12;
  array[2] = 24;

  auto newarray = array;
  EXPECT_EQ(newarray[0], 12);
}

TEST_F(LazyArrayTest, SerializationVer0) {
  std::stringstream ss(
      "22 serialization::archive 20 0 0 10 1 1 0\n"
      "0 0 -1 1\n"
      "1 2 -1 1\n"
      "2 4 -1 1\n"
      "3 6 -1 1\n"
      "4 8 -1\n");

  {
    LazyArray<IntWrapper> newArray(SIZE);
    boost::archive::text_iarchive ia(ss);
    ia >> newArray;
    checkArray(newArray);
  }
}

TEST_F(LazyArrayTest, SerializationVer1) {
  {
    std::stringstream ss(
        "22 serialization::archive 20 0 0 -1 10 5 0 1 0\n"
        "0 0 2\n"
        "1 2 4\n"
        "2 4 6\n"
        "3 6 8\n"
        "4 8\n");
    boost::archive::text_iarchive ia(ss);
    LazyArray<IntWrapper> arr(SIZE);
    ia >> arr;
    checkArray(arr);
  }

  // {
  //   std::stringstream ss(
  //       "22 serialization::archive 20 0 0 -1 10 5 0 0 2 2 4 4 6 6 8 8");
  //   boost::archive::text_iarchive ia(ss);
  //   LazyArray<int> arr;
  //   ia >> arr;
  //   for (int i = 0; i < SIZE; ++i) {
  //     if (i % 2 == 0) {
  //       EXPECT_EQ(arr[i], i);
  //     } else {
  //       EXPECT_FALSE(arr.Exists(i));
  //     }
  //   }
  // }
  // currently cannot compile
}
