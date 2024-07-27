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

#ifndef SRC_LIBREALLIVE_ELEMENTS_TEXTOUT_H_
#define SRC_LIBREALLIVE_ELEMENTS_TEXTOUT_H_

#include <ostream>
#include <string>

#include "libreallive/elements/bytecode.h"

namespace libreallive {

class TextoutFactory;

// Display-text elements.
class TextoutElement : public BytecodeElement {
 protected:
  friend class Parser;
  TextoutElement(const char* src, const char* file_end);

 public:
  virtual ~TextoutElement();

  const std::string GetText() const;

  // Overridden from BytecodeElement::
  virtual void PrintSourceRepresentation(RLMachine* machine,
                                         std::ostream& oss) const final;
  virtual const size_t GetBytecodeLength() const final;
  virtual void RunOnMachine(RLMachine& machine) const final;

 private:
  std::string repr;
};

}  // namespace libreallive

#endif
