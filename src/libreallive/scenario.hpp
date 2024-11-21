// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------

#pragma once

#include <string>
#include <string_view>

#include "libreallive/alldefs.hpp"
#include "libreallive/header.hpp"
#include "libreallive/script.hpp"
#include "utilities/mapped_file.hpp"

namespace libreallive {

class Scenario {
 public:
  Scenario(Header hdr, Script scr, int num);
  ~Scenario();

  // Get the scenario number
  int scene_number() const { return scenario_number_; }

  // Get the text encoding used for this scenario
  int encoding() const { return header.rldev_metadata_.text_encoding(); }

  // Access to metadata in the script. Don't worry about information loss;
  // valid values are 0, 1, and 2.
  int savepoint_message() const { return header.savepoint_message_; }
  int savepoint_selcom() const { return header.savepoint_selcom_; }
  int savepoint_seentop() const { return header.savepoint_seentop_; }

  Header header;
  Script script;
  int scenario_number_;
};

std::unique_ptr<Scenario> ParseScenario(FilePos fp,
                                        int scenarioNum,
                                        const std::string& regname,
                                        const XorKey* second_level_xor_key);

}  // namespace libreallive
