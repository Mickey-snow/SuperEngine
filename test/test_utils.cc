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

#include "test_utils.h"
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------

const std::vector<std::string> testPaths = {"./", "./build/test/", "./test/"};

std::string locateTestCase(const std::string& baseName) {
  for (const auto& it : testPaths) {
    fs::path testName = fs::path(it) / baseName;
    if (fs::exists(testName))
      return testName.string();
  }

  std::ostringstream oss;
  oss << "Could not locate data file '" << baseName << "' in locateTestCase.";
  throw std::runtime_error(oss.str());
}

std::string locateTestDirectory(const std::string& baseName) {
  for (const auto& it : testPaths) {
    fs::path dirName = fs::path(it) / baseName;
    if (fs::is_directory(dirName))
      return dirName.string();
  }

  throw std::runtime_error("Could not locate directory " + baseName);
}

// -----------------------------------------------------------------------

FullSystemTest::FullSystemTest()
    : arc(locateTestCase("Module_Str_SEEN/strcpy_0.TXT")),
      system(locateTestCase("Gameexe_data/Gameexe.ini")),
      rlmachine(system, arc) {}

FullSystemTest::~FullSystemTest() {}
