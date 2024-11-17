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

#include "libreallive/elements/bytecode.hpp"

#include <memory>

namespace libreallive {
// Metadata elements: source line, kidoku, and entrypoint markers.
class MetaElement : public BytecodeElement {
 public:
  enum MetaElementType { Line_ = '\n', Kidoku_ = '@', Entrypoint_ };

 public:
  MetaElement(const MetaElementType& type,
              const int& value,
              const int& entrypoint_index);
  virtual ~MetaElement();

  const int value() const { return value_; }
  void set_value(const int value) { value_ = value; }

  std::string GetSourceRepresentation(IModuleManager*) const;

  // Overridden from BytecodeElement:
  virtual void PrintSourceRepresentation(IModuleManager* machine,
                                         std::ostream& oss) const final;
  virtual const size_t GetBytecodeLength() const final;
  virtual const int GetEntrypoint() const final;

  virtual Bytecode_ptr DownCast() const final;

  MetaElementType type_;
  int value_;
  int entrypoint_index_;
};

}  // namespace libreallive
