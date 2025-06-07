// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#pragma once

#include "vm/value_fwd.hpp"

#include <sstream>
#include <string>
#include <unordered_set>

namespace serilang {

class Disassembler {
 public:
  Disassembler(size_t indent = 2);

  // pretty–prints a Code’s byte-code
  std::string Dump(Code& chunk);

 private:
  // print a single instruction in one row
  void PrintIns(Code& chunk, size_t& ip, const std::string& indent);
  // recursively disassemble a chunk
  void DumpImpl(Code& chunk, const std::string& indent);

  size_t indent_size_;

  std::ostringstream out_;
  std::unordered_set<const Code*> seen_;
};

}  // namespace serilang
