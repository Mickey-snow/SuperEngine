// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#ifndef SRC_LIBREALLIVE_SCENARIO_H_
#define SRC_LIBREALLIVE_SCENARIO_H_

#include <string>
#include <string_view>

#include "libreallive/alldefs.h"
#include "libreallive/parser.h"
#include "utilities/mapped_file.h"

namespace libreallive {

struct XorKey;

class Metadata {
 public:
  Metadata();
  const string& to_string() const { return as_string_; }
  const int text_encoding() const { return encoding_; }

  void Assign(const char* input);

  void Assign(const std::string_view& input);

 private:
  std::string as_string_;
  int encoding_;
};

class Header {
 public:
  Header(const char* const data, const size_t length);
  Header(const std::string_view& data) : Header(data.data(), data.length()) {}
  ~Header();

  // Starting around the release of Little Busters!, scenario files has a
  // second round of xor done to them. When will they learn?
  bool use_xor_2_;

  long z_minus_one_;
  long z_minus_two_;
  long savepoint_message_;
  long savepoint_selcom_;
  long savepoint_seentop_;
  std::vector<string> dramatis_personae_;
  Metadata rldev_metadata_;
};

class Script {
 public:
  const pointer_t GetEntrypoint(int entrypoint) const;

 private:
  friend class Scenario;

  Script(const Header& hdr,
         const char* const data,
         const size_t length,
         const std::string& regname,
         bool use_xor_2,
         const XorKey* second_level_xor_key);
  Script(const Header& hdr,
         const std::string_view& data,
         const std::string& regname,
         bool use_xor_2,
         const XorKey* second_level_xor_key);
  ~Script();

  // A sequence of semi-parsed/tokenized bytecode elements, which are
  // the elements that RLMachine executes.
  BytecodeList elts_;

  // Entrypoint handeling
  typedef std::map<int, pointer_t> pointernumber;
  pointernumber entrypoint_associations_;
};

class Scenario {
 public:
  Scenario(const std::string_view& data,
           int scenarioNum,
           const std::string& regname,
           const XorKey* second_level_xor_key);
  Scenario(FilePos fp,
           int scenarioNum,
           const std::string& regname,
           const XorKey* second_level_xor_key);
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

  // Access to script
  typedef BytecodeList::const_iterator const_iterator;
  typedef BytecodeList::iterator iterator;

  const_iterator begin() const { return script.elts_.cbegin(); }
  const_iterator end() const { return script.elts_.cend(); }

  // Locate the entrypoint
  const_iterator FindEntrypoint(int entrypoint) const;

 private:
  Header header;
  Script script;
  int scenario_number_;
};

}  // namespace libreallive

#endif  // SRC_LIBREALLIVE_SCENARIO_H_
