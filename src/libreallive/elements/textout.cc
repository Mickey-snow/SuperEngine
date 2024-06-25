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

#include "libreallive/elements/textout.h"
#include "machine/rlmachine.h"

namespace libreallive {
// -----------------------------------------------------------------------
// TextoutElement
// -----------------------------------------------------------------------

TextoutElement::TextoutElement(const char* src, const char* end) {
  repr.assign(src, end);
}

TextoutElement::~TextoutElement() {}

const string TextoutElement::GetText() const {
  string rv;
  bool quoted = false;
  string::const_iterator it = repr.cbegin();
  while (it != repr.cend()) {
    if (*it == '"') {
      ++it;
      quoted = !quoted;
    } else if (quoted && *it == '\\') {
      ++it;
      if (*it == '"') {
        ++it;
        rv.push_back('"');
      } else {
        rv.push_back('\\');
      }
    } else {
      if ((*it >= 0x81 && *it <= 0x9f) || (*it >= 0xe0 && *it <= 0xef))
        rv.push_back(*it++);
      rv.push_back(*it++);
    }
  }
  return rv;
}

void TextoutElement::PrintSourceRepresentation(RLMachine* machine,
                                               std::ostream& oss) const {
  oss << "\"" << GetText() << "\"" << std::endl;
}

const size_t TextoutElement::GetBytecodeLength() const { return repr.size(); }

void TextoutElement::RunOnMachine(RLMachine& machine) const {
  machine.PerformTextout(*this);
  machine.AdvanceInstructionPointer();
}

// -----------------------------------------------------------------------
// TextoutFactory
// -----------------------------------------------------------------------

extern char entrypoint_marker;

// static
TextoutElement* TextoutFactory::Read(const char* src, const char* file_end) {
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
