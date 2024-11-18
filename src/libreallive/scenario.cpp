// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006 Peter Jolly
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

#include "libreallive/scenario.hpp"

#include "utilities/exception.hpp"
#include "utilities/string_utilities.hpp"

namespace libreallive {

Scenario::Scenario(const std::string_view& data,
                   int sn,
                   const std::string& regname,
                   const XorKey* second_level_xor_key)
    : header(data),
      script(header, data, regname, header.use_xor_2_, second_level_xor_key),
      scenario_number_(sn) {}

Scenario::Scenario(FilePos fp,
                   int sn,
                   const std::string& regname,
                   const XorKey* second_level_xor_key)
    : Scenario(fp.Read(), sn, regname, second_level_xor_key) {}

Scenario::~Scenario() {}

Scenario::const_iterator Scenario::FindEntrypoint(int entrypoint) const {
  return script.GetEntrypoint(entrypoint);
}

}  // namespace libreallive
