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
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "object/parameter_manager.h"

class ScapegoatTest : public ::testing::Test {
 protected:
  Scapegoat paramManager;
};

TEST_F(ScapegoatTest, InsertAndRetrieve) {
  paramManager.Set("health", 100);
  paramManager.Set("name", std::string("Player1"));

  struct CustomType {
    int x;
    float y;
  };
  paramManager.Set("custom", CustomType{42, 3.14f});

  {
    std::any value = paramManager["health"];
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(std::any_cast<int>(value), 100);
  }

  {
    std::any value = paramManager["name"];
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(std::any_cast<std::string>(value), "Player1");
  }

  {
    std::any value = paramManager["custom"];
    EXPECT_TRUE(value.has_value());
    CustomType retrievedValue = std::any_cast<CustomType>(value);
    EXPECT_EQ(retrievedValue.x, 42);
    EXPECT_FLOAT_EQ(retrievedValue.y, 3.14f);
  }
}

TEST_F(ScapegoatTest, RetrieveNonExistentKey) {
  paramManager.Set("some_key", 123);
  EXPECT_THROW(
      { int value = std::any_cast<int>(paramManager["non_existent_key"]); },
      std::out_of_range);
}

TEST_F(ScapegoatTest, ModifyValue) {
  paramManager.Set("score", 50);
  paramManager.Set("score", 75);
  std::any value = paramManager["score"];
  EXPECT_EQ(std::any_cast<int>(value), 75);
}

TEST_F(ScapegoatTest, CopyOnWrite) {
  static size_t copy_count = 0, move_count = 0;
  struct IntValue {
    int value;
    IntValue(int ival) : value(ival) {}
    IntValue(const IntValue& rhs) : value(rhs.value) { ++copy_count; }
    IntValue(IntValue&& rhs) : value(rhs.value) { ++move_count; }
    IntValue& operator=(const IntValue& rhs) {
      value = rhs.value;
      ++copy_count;
      return *this;
    }
    IntValue& operator=(IntValue&& rhs) {
      value = rhs.value;
      ++move_count;
      return *this;
    }
  };

  paramManager.Set("root", IntValue(1));
  for (int i = 1; i <= 1000; ++i)
    paramManager.Set("member" + std::to_string(i), IntValue(i + 100));
  copy_count = move_count = 0;

  Scapegoat paramManagerCopy = paramManager;
  paramManagerCopy.Set("root", IntValue(2));
  EXPECT_EQ(std::any_cast<IntValue>(paramManager["root"]).value, 1);
  EXPECT_EQ(std::any_cast<IntValue>(paramManagerCopy["root"]).value, 2);
  EXPECT_LE(copy_count, 14);
}

TEST_F(ScapegoatTest, Persistence) {
  paramManager.Set("experience", 1000);

  Scapegoat version1 = paramManager;
  paramManager.Set("experience", 2000);
  Scapegoat version2 = paramManager;
  paramManager.Set("experience", 3000);

  EXPECT_EQ(std::any_cast<int>(version1["experience"]), 1000);
  EXPECT_EQ(std::any_cast<int>(version2["experience"]), 2000);
  EXPECT_EQ(std::any_cast<int>(paramManager["experience"]), 3000);
}

TEST_F(ScapegoatTest, RemoveKey) {
  paramManager.Set("temp", 42);
  EXPECT_TRUE(paramManager.Contains("temp"));
  paramManager.Remove("temp");
  EXPECT_FALSE(paramManager.Contains("temp"));
  EXPECT_THROW({ std::any value = paramManager["temp"]; }, std::out_of_range);
}

TEST_F(ScapegoatTest, StressTest) {
  const int num_entries = 1000;

  std::vector<std::string> keys;
  std::vector<std::any> values;

  for (int i = 0; i < num_entries; ++i) {
    keys.push_back("key_" + std::to_string(i));

    switch (i % 5) {
        case 0:
        values.push_back(i);
        break;
        case 1:
          values.push_back(static_cast<double>(i) * 0.1);
          break;
        case 2:
          values.push_back("string_" + std::to_string(i));
          break;
        case 3:
          values.push_back(std::vector<int>{i, i + 1, i + 2});
          break;
        case 4:
          values.push_back(std::map<std::string, int>{{"a", i}, {"b", i +
          1}}); break;
    }
  }

  for (int i = 0; i < num_entries; ++i) {
    paramManager.Set(keys[i], values[i]);
  }

  for (int i = 0; i < num_entries; ++i) {
    std::any retrieved_value = paramManager[keys[i]];
    ASSERT_TRUE(retrieved_value.has_value());

    switch (i % 5) {
      case 0:
        EXPECT_EQ(std::any_cast<int>(retrieved_value),
                  std::any_cast<int>(values[i]));
        break;
      case 1:
        EXPECT_DOUBLE_EQ(std::any_cast<double>(retrieved_value),
                         std::any_cast<double>(values[i]));
        break;
      case 2:
        EXPECT_EQ(std::any_cast<std::string>(retrieved_value),
                  std::any_cast<std::string>(values[i]));
        break;
      case 3:
        EXPECT_EQ(std::any_cast<std::vector<int>>(retrieved_value),
                  std::any_cast<std::vector<int>>(values[i]));
        break;
      case 4:
        EXPECT_EQ((std::any_cast<std::map<std::string,
        int>>(retrieved_value)),
                  (std::any_cast<std::map<std::string, int>>(values[i])));
        break;
    }
  }

  EXPECT_THROW(paramManager["non_existent_key"], std::out_of_range);
}
