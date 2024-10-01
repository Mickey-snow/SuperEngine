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

#ifndef SRC_PLATFORM_IMPLEMENTOR_H_
#define SRC_PLATFORM_IMPLEMENTOR_H_

#include <filesystem>
#include <string>

class IPlatformImplementor {
 public:
  virtual ~IPlatformImplementor() = default;

  // ask the user to select game directory, return path to directory root.
  virtual std::filesystem::path SelectGameDirectory() = 0;

  // report error before program crashes
  virtual void ReportFatalError(const std::string& message_text,
                                const std::string& informative_text) = 0;

  // ask the user to select a yes/no
  virtual bool AskUserPrompt(const std::string& message_text,
                             const std::string& informative_text,
                             const std::string& true_button,
                             const std::string& false_button) = 0;
};

class DefaultPlatformImpl : public IPlatformImplementor {
 public:
  DefaultPlatformImpl() = default;
  virtual ~DefaultPlatformImpl() = default;

  // when this has no implementation, simply return an empty path
  virtual std::filesystem::path SelectGameDirectory() override { return {}; }

  virtual void ReportFatalError(const std::string& message_test,
                                const std::string& informative_text) override {}

  virtual bool AskUserPrompt(const std::string&,
                             const std::string&,
                             const std::string&,
                             const std::string&) override {
    return true;
  }
};

#endif
