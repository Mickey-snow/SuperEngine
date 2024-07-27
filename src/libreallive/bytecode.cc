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
#include "libreallive/expression.h"
#include "libreallive/scenario.h"
#include "machine/rlmachine.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

namespace libreallive {

std::string ParsableToPrintableString(const std::string& src) {
  string output;

  bool firstToken = true;
  for (char tok : src) {
    if (firstToken) {
      firstToken = false;
    } else {
      output += " ";
    }

    if (tok == '(' || tok == ')' || tok == '$' || tok == '[' || tok == ']') {
      output.push_back(tok);
    } else {
      std::ostringstream ss;
      ss << std::hex << std::setw(2) << std::setfill('0') << int(tok);
      output += ss.str();
    }
  }

  return output;
}

std::string PrintableToParsableString(const std::string& src) {
  typedef boost::tokenizer<boost::char_separator<char>> ttokenizer;

  std::string output;

  boost::char_separator<char> sep(" ");
  ttokenizer tokens(src, sep);
  for (string const& tok : tokens) {
    if (tok.size() > 2)
      throw libreallive::Error(
          "Invalid string given to printableToParsableString");

    if (tok == "(" || tok == ")" || tok == "$" || tok == "[" || tok == "]") {
      output.push_back(tok[0]);
    } else {
      int charToAdd;
      std::istringstream ss(tok);
      ss >> std::hex >> charToAdd;
      output.push_back((char)charToAdd);
    }
  }

  return output;
}

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
      Expression piece(GetData(start));
      oss << piece->GetDebugString();
    } catch (libreallive::Error& e) {
      // Any error throw here is a parse error.
      oss << "{RAW : " << ParsableToPrintableString(param) << "}";
    }
  }
  oss << ")";
}

char entrypoint_marker = '@';

// static
BytecodeElement* Parser::ParseBytecode(const char* stream,
                                       const char* end,
                                       ConstructionData& cdata) {
  const char c = *stream;
  if (c == '!')
    entrypoint_marker = '!';

  BytecodeElement* result = nullptr;
  switch (c) {
    case 0:
    case ',':
      result = new CommaElement;
      break;
    case '\n':
      result = new MetaElement(nullptr, stream);
      break;
    case '@':
    case '!':
      result = new MetaElement(&cdata, stream);
      break;
    case '$':
      result = new ExpressionElement(stream);
      break;
    case '#':
      result = FunctionFactory::ReadFunction(stream, cdata);
      break;
    default:
      result = ParseTextout(stream, end);
      break;
  }

  // result->PrintSourceRepresentation(nullptr, std::cout);
  // std::string rawbytes(stream, result->GetBytecodeLength());
  // std::cout<<ParsableToPrintableString(rawbytes)<<std::endl<<std::endl;
  return result;
}

TextoutElement* Parser::ParseTextout(const char* src, const char* file_end) {
  const char* end = src;
  bool quoted = false;
  while (end < file_end) {
    if (quoted) {
      quoted = *end != '"';
      if (*end == '\\' && end[1] == '"')  // escaped quote
        ++end;
    } else {
      if (*end == ',')  // not a comma element
        ++end;
      quoted = *end == '"';

      // new element
      if (!*end || *end == '#' || *end == '$' || *end == '\n' || *end == '@' ||
          *end == entrypoint_marker)
        break;
    }

    if ((*end >= 0x81 && *end <= 0x9f) || (*end >= 0xe0 && *end <= 0xef))
      end += 2;  // shift.jis
    else
      ++end;
  }
  return new TextoutElement(src, end);
}

}  // namespace libreallive
