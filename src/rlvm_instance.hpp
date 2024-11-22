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

class Platform;
class RLMachine;
class System;
class IPlatformImplementor;

// The main, cross platform emulator class. Has template methods for
// implementing platform specific GUI.
class RLVMInstance {
 public:
  RLVMInstance();
  virtual ~RLVMInstance();

  // Runs the main emulation loop.
  void Run(const std::filesystem::path& gamepath);

  void set_seen_start(int in) { seen_start_ = in; }
  void set_memory() { memory_ = true; }
  void set_undefined_opcodes() { undefined_opcodes_ = true; }
  void set_count_undefined() { count_undefined_copcodes_ = true; }
  void set_tracing() { tracing_ = true; }
  void set_load_save(int in) { load_save_ = in; }
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
  // Finds a game file, causing an error if not found.
  std::filesystem::path FindGameFile(const std::filesystem::path& gamerootPath,
                                     const std::string& filename);

  // Checks to see if the user ran the Japanese version and than installed a
  // fan patch. In this case, we need to warn and let the user reset global
  // data.
  void DoUserNameCheck(RLMachine& machine);

  // Checks for AVG32/Siglus engine games, which people may be confused about.
  void CheckBadEngine(const std::filesystem::path& gamerootPath,
                      const std::vector<std::string> filenames,
                      const std::string& message_text);

  // Whether we should set a custom font.
  std::string custom_font_;

  // Which SEEN# we should start execution from (-1 if we shouldn't set this).
  int seen_start_;

  // Whether we should force '#MEMORY=1' to enter debug mode.
  bool memory_;

  // Whether we should print undefined opcode messages.
  bool undefined_opcodes_;

  // Whether we should print out a table of undefined opcodes that the game
  // used on exit.
  bool count_undefined_copcodes_;

  // Whether we should print out the opcodes as they are running.
  bool tracing_;

  // Loads the specified save file as soon as emulation starts if not -1.
  int load_save_;

  // The bridge to the class that implements platform-specific code
  std::shared_ptr<IPlatformImplementor> platform_implementor_;
};
