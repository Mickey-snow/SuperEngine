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

#ifndef SRC_LIBREALLIVE_ELEMENTS_BYTECODE_H_
#define SRC_LIBREALLIVE_ELEMENTS_BYTECODE_H_

#include <ostream>
#include <string>
#include <map>
#include <vector>

#include "libreallive/bytecode_fwd.h"

class RLMachine;

namespace libreallive{
  class Script;
  
  struct ConstructionData {
  ConstructionData(size_t kt, pointer_t pt);
  ~ConstructionData();

  std::vector<unsigned long> kidoku_table;
  pointer_t null;
  typedef std::map<unsigned long, pointer_t> offsets_t;
  offsets_t offsets;
};

// Base classes for bytecode elements.
class BytecodeElement {
 public:
  static const int kInvalidEntrypoint = -999;

  BytecodeElement();
  virtual ~BytecodeElement();

  // Prints a human readable version of this bytecode element to |oss|. This
  // tries to match Haeleth's kepago language as much as is feasible.
  virtual void PrintSourceRepresentation(RLMachine* machine,
                                         std::ostream& oss) const;

  // Returns the length of this element in bytes in the source file.
  virtual const size_t GetBytecodeLength() const = 0;

  // Used to connect pointers in the bytecode after we've created all
  // BytecodeElements in a Scenario.
  virtual void SetPointers(ConstructionData& cdata);

  // Needed for MetaElement during reading the script
  virtual const int GetEntrypoint() const;

  // Fat interface: takes a FunctionElement and returns all data serialized for
  // writing to disk so the exact command can be replayed later. Throws in all
  // other cases.
  virtual std::string GetSerializedCommand(RLMachine& machine) const;

  // Execute this bytecode instruction on this virtual machine
  virtual void RunOnMachine(RLMachine& machine) const;

 protected:
  BytecodeElement(const BytecodeElement& c);

 private:
  friend class Script;
};
}

#endif
