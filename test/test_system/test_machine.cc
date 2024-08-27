// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#include "test_system/test_machine.h"

#include <sstream>
#include <string>
#include <vector>

#include "libreallive/alldefs.h"
#include "machine/rlmodule.h"
#include "machine/rloperation.h"
#include "utilities/exception.h"
#include "xclannad/endian.hpp"

using libreallive::insert_i16;
using rlvm::Exception;
using std::make_pair;

TestMachine::TestMachine(System& in_system, libreallive::Archive& in_archive)
    : RLMachine(in_system, in_archive) {}

void TestMachine::AttachModule(RLModule* module) {
  for (RLModule::OpcodeMap::iterator it = module->begin(); it != module->end();
       ++it) {
    int opcode = -1;
    unsigned char overload = 0;
    RLModule::UnpackOpcodeNumber(it->first, opcode, overload);
    if (opcode == 77 || opcode == 1057) {
      // XXX: recMulti and grpMulti each have two variants with different
      // opcodes and argument types. There's no way to tell them apart using
      // just op name and overload index so we're gonna just skip registering
      // the variants that are not used in tests.
      continue;
    }

    RLOperation* op = it->second.get();

    auto key = make_pair(op->name(), overload);
    if (registry_.count(key) > 0) {
      std::ostringstream ss;
      ss << "Duplicate registry key (" << op->name()
         << ", " << static_cast<int>(overload) << ")";
      throw rlvm::Exception(ss.str());
    }
    registry_.emplace(std::move(key), op);
  }

  RLMachine::AttachModule(module);
}

void TestMachine::Exe(const std::string& name, unsigned char overload) {
  RunOpcode(name, overload, 0, "");
}

void TestMachine::Exe(const std::string& name,
                      unsigned char overload,
                      const ExeArgument& arguments) {
  RunOpcode(name, overload, arguments.first, arguments.second);
}

// static
void TestMachine::AddEntity(std::string& output, const std::string& arg) {
  output += "\"";
  output += arg;
  output += "\"";
}

// static
void TestMachine::AddEntity(std::string& output, const int arg) {
  char buf[6];
  buf[0] = '$';
  buf[1] = 0xff;
  write_little_endian_int(buf + 2, arg);
  output.append(buf, 6);
}

void TestMachine::RunOpcode(const std::string& name,
                            unsigned char overload,
                            int argc,
                            const std::string& argument_string) {
  string repr;
  repr.resize(8, 0);
  repr[0] = '#';
  repr[1] = 0;             // type
  repr[2] = 0;             // module
  insert_i16(repr, 3, 0);  // opcode
  insert_i16(repr, 5, argc);
  repr[7] = overload;

  string full = repr + '(' + argument_string + ')';
  std::unique_ptr<libreallive::CommandElement> element(
      libreallive::CommandParser::ParseNormalFunction(full.c_str()));
  // (TODO) There should be an interface to create CommandElement easily

  RLOperation* op = registry_[make_pair(name, overload)];
  if (op) {
    op->DispatchFunction(*this, *element.get());
  } else {
    throw rlvm::Exception("Illegal opcode TestMachine::runOpcode");
  }
}
