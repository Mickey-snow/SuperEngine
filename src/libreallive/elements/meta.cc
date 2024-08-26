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

#include "libreallive/elements/meta.h"
#include "machine/rlmachine.h"

namespace libreallive {

MetaElement::MetaElement(std::shared_ptr<ConstructionData> cv,
                         const char* src) {
  value_ = read_i16(src + 1);
  if (!cv) {
    type_ = Line_;
  } else if (cv->kidoku_table.at(value_) >= 1000000) {
    type_ = Entrypoint_;
    entrypoint_index_ = cv->kidoku_table[value_] - 1000000;
  } else {
    type_ = Kidoku_;
  }
}

MetaElement::~MetaElement() {}

void MetaElement::PrintSourceRepresentation(RLMachine* machine,
                                            std::ostream& oss) const {
  if (type_ == Line_)
    oss << "#line " << value_ << std::endl;
  else if (type_ == Entrypoint_)
    oss << "#entrypoint " << value_ << std::endl;
  else
    oss << "{- Kidoku " << value_ << " -}" << std::endl;
}

const size_t MetaElement::GetBytecodeLength() const { return 3; }

const int MetaElement::GetEntrypoint() const {
  return type_ == Entrypoint_ ? entrypoint_index_ : kInvalidEntrypoint;
}

void MetaElement::RunOnMachine(RLMachine& machine) const {
  if (type_ == Line_)
    machine.SetLineNumber(value_);
  else if (type_ == Kidoku_)
    machine.SetKidokuMarker(value_);

  machine.AdvanceInstructionPointer();
}

}  // namespace libreallive
