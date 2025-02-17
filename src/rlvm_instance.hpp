// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2011 Elliot Glaysher
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

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Gameexe;
class Platform;
class RLMachine;
class System;
class Debugger;
class IPlatformImplementor;
namespace libreallive {
class Archive;
}

// The main, cross platform emulator class. Has template methods for
// implementing platform specific GUI.
class RLVMInstance {
 public:
  RLVMInstance();
  virtual ~RLVMInstance();

  void Main(const std::filesystem::path& gamepath);

  void set_seen_start(int in) { seen_start_ = in; }
  void set_custom_font(const std::string& font) { custom_font_ = font; }

  void SetPlatformImplementor(std::shared_ptr<IPlatformImplementor> impl);

 protected:
  // Should bring up a platform native dialog box to report the message.
  virtual void ReportFatalError(const std::string& message_text,
                                const std::string& informative_text);

  // Ask the user if we should take an action.
  virtual bool AskUserPrompt(const std::string& message_text,
                             const std::string& informative_text,
                             const std::string& true_button,
                             const std::string& false_button);

 private:
  // Control the machine to execute the next operation
  void Step();

  // Returns a formatted string to descirbe current location (scene number,
  // line)
  std::string DescribeCurrentIP() const;

  std::shared_ptr<System> system_;

  std::shared_ptr<RLMachine> machine_;

  std::shared_ptr<Debugger> debugger_;

  // Whether we should set a custom font.
  std::string custom_font_;

  // Which SEEN# we should start execution from (-1 if we shouldn't set this).
  int seen_start_;

  // The bridge to the class that implements platform-specific code
  std::shared_ptr<IPlatformImplementor> platform_implementor_;
};
