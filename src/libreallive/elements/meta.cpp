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

#include "libreallive/elements/meta.hpp"

namespace libreallive {

MetaElement::MetaElement(const MetaElementType& type,
                         const int& value,
                         const int& entrypoint_index)
    : type_(type), value_(value), entrypoint_index_(entrypoint_index) {}

MetaElement::~MetaElement() {}

std::string MetaElement::GetSourceRepresentation(IModuleManager*) const {
  using std::string_literals::operator""s;

  if (type_ == Line_)
    return "#line "s + std::to_string(value_);
  else if (type_ == Entrypoint_)
    return "#entrypoint "s + std::to_string(value_);
  else
    return "{- Kidoku "s + std::to_string(value_) + " -}";
}

void MetaElement::PrintSourceRepresentation(IModuleManager* machine,
                                            std::ostream& oss) const {
  oss << GetSourceRepresentation(machine) << std::endl;
}

size_t MetaElement::GetBytecodeLength() const { return 3; }

int MetaElement::GetEntrypoint() const {
  return type_ == Entrypoint_ ? entrypoint_index_ : kInvalidEntrypoint;
}

Bytecode_ptr MetaElement::DownCast() const { return this; }

}  // namespace libreallive
