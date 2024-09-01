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

#include "utilities/mapped_file.h"

#include <boost/filesystem.hpp>
#include <filesystem>
#include <fstream>

namespace fs = boost::filesystem;

// Helper function to create a temporary file for testing
std::string createTemporaryFile(const std::string& content) {
  fs::path temp = fs::temp_directory_path() / fs::unique_path();
  std::ofstream file(temp.string());
  file << content;
  file.close();
  return temp.string();
}

class MappedFileTest : public ::testing::Test {
 protected:
  std::string test_file_path;

  void SetUp() override {
    test_file_path = createTemporaryFile("Hello, world!");
  }

  void TearDown() override {
    if (fs::exists(test_file_path)) {
      fs::remove(test_file_path);
    }
  }
};

TEST_F(MappedFileTest, ConstructorReadOnly) {
  MappedFile file(test_file_path);
  EXPECT_EQ(file.Size(), 13);  // "Hello, world!" is 13 characters long
}

TEST_F(MappedFileTest, ConstructorReadWrite) {
  MappedFile file(test_file_path, 100);
  EXPECT_EQ(file.Size(), 100);
}

TEST_F(MappedFileTest, ReadFullFile) {
  MappedFile file(test_file_path);
  std::string_view data = file.Read();
  EXPECT_EQ(data, "Hello, world!");
}

TEST_F(MappedFileTest, ReadPartialFile) {
  MappedFile file(test_file_path);
  std::string_view data = file.Read(7, 5);
  EXPECT_EQ(data, "world");
}

TEST_F(MappedFileTest, ReadOutOfBounds) {
  MappedFile file(test_file_path);
  EXPECT_THROW(file.Read(50), std::out_of_range);
}

TEST_F(MappedFileTest, WriteToFile) {
  MappedFile file(test_file_path, 100);
  EXPECT_NO_THROW(file.Write(0, "Boost"));
  std::string_view data = file.Read(0, 5);
  EXPECT_EQ(data, "Boost");
}

TEST_F(MappedFileTest, WriteOutOfBounds) {
  MappedFile file(test_file_path, 100);
  EXPECT_THROW(file.Write(96, "Boost"), std::out_of_range)
      << " exception should be thrown when writing \"Boost\" at position 96, "
         "which exceeds the file size";
}

TEST_F(MappedFileTest, WriteNoPermission) {
  MappedFile file(test_file_path);
  EXPECT_THROW(file.Write(0, "Boost"), std::runtime_error)
      << "exception should be thrown when writing with no write permission";
}

TEST_F(MappedFileTest, UnopenedFile) {
  std::string invalid_file = "";
  EXPECT_THROW(
      {
        MappedFile file(invalid_file);
        file.Read(0);
      },
      std::exception)
      << "exception should be thrown when accessing an unopened file";
}
