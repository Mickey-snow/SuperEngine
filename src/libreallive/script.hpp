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

#include "libreallive/bytecode_fwd.hpp"
#include "libreallive/header.hpp"

#include <map>
#include <memory>

namespace libreallive {

class Script {
 public:
  Script(BytecodeList elts,
         std::map<int, pointer_t> __entrypoints,
         std::vector<std::pair<unsigned long, std::shared_ptr<BytecodeElement>>>
             elements,
         std::map<int, unsigned long> entrypoints);
  ~Script();

  const pointer_t GetEntrypoint(int entrypoint) const;

  // A sequence of semi-parsed/tokenized bytecode elements, which are
  // the elements that RLMachine executes.
  BytecodeList elts_;

  // Entrypoint handeling
  std::map<int, pointer_t> entrypoint_associations_;

  std::vector<std::pair<unsigned long, std::shared_ptr<BytecodeElement>>>
      elements_;
  std::map<int, unsigned long> entrypoints_;
};

Script ParseScript(const Header& hdr,
                   const std::string_view& data,
                   const std::string& regname,
                   bool use_xor_2,
                   const XorKey* second_level_xor_key);

}  // namespace libreallive
