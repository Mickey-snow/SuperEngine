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

#include "libreallive/elements/bytecode.hpp"
#include "machine/rlmachine.hpp"

namespace libreallive {

// -----------------------------------------------------------------------
// ConstructionData
// -----------------------------------------------------------------------

ConstructionData::ConstructionData(size_t kt, pointer_t pt)
    : kidoku_table(kt), null(pt) {}

ConstructionData::~ConstructionData() {}

// -----------------------------------------------------------------------
// BytecodeElement
// -----------------------------------------------------------------------

BytecodeElement::BytecodeElement() {}

BytecodeElement::~BytecodeElement() {}

BytecodeElement::BytecodeElement(const BytecodeElement& c) {}

void BytecodeElement::PrintSourceRepresentation(IModuleManager* manager,
                                                std::ostream& oss) const {
  oss << GetSourceRepresentation(manager) << std::endl;
}

std::string BytecodeElement::GetSourceRepresentation(IModuleManager*) const {
  return "<unspecified bytecode>";
}

void BytecodeElement::SetPointers(ConstructionData& cdata) {}

const int BytecodeElement::GetEntrypoint() const { return kInvalidEntrypoint; }

string BytecodeElement::GetSerializedCommand(RLMachine& machine) const {
  throw Error(
      "Can't call GetSerializedCommand() on things other than "
      "FunctionElements");
}

void BytecodeElement::RunOnMachine(RLMachine& machine) const {
  machine.AdvanceInstructionPointer();
}

}  // namespace libreallive
