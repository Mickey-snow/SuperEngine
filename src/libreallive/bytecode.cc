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

#include "libreallive/bytecode.h"

#include <cassert>
#include <cstring>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "libreallive/expression.h"
#include "libreallive/scenario.h"

#include "machine/rlmachine.h"

namespace libreallive {

void PrintParameterString(std::ostream& oss,
                          const std::vector<std::string>& parameters) {
  bool first = true;
  oss << "(";
  for (std::string const& param : parameters) {
    if (!first) {
      oss << ", ";
    }
    first = false;

    // Take the binary stuff and try to get usefull, printable values.
    const char* start = param.c_str();
    try {
      ExpressionPiece piece(GetData(start));
      oss << piece.GetDebugString();
    } catch (libreallive::Error& e) {
      // Any error throw here is a parse error.
      oss << "{RAW : " << ParsableToPrintableString(param) << "}";
    }
  }
  oss << ")";
}

char entrypoint_marker = '@';

// static
BytecodeElement* BytecodeFactory::Read(const char* stream,
                                       const char* end,
                                       ConstructionData& cdata) {
  const char c = *stream;
  if (c == '!')
    entrypoint_marker = '!';
  switch (c) {
    case 0:
    case ',':
      return new CommaElement;
    case '\n':
      return new MetaElement(nullptr, stream);
    case '@':
    case '!':
      return new MetaElement(&cdata, stream);
    case '$':
      return new ExpressionElement(stream);
    case '#':
      return FunctionFactory::ReadFunction(stream, cdata);
    default:
      return TextoutFactory::Read(stream, end);
  }
}

}  // namespace libreallive
