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

#ifndef SRC_LIBREALLIVE_ELEMENTS_EXPRESSION_H_
#define SRC_LIBREALLIVE_ELEMENTS_EXPRESSION_H_

#include "libreallive/elements/bytecode.h"
#include "libreallive/expression.h"

namespace libreallive{
  // Expression elements.
// Construct from long to build a representation of an integer constant.

// A BytecodeElement that represents an expression
class ExpressionElement : public BytecodeElement {
 public:
  explicit ExpressionElement(const long val);
  explicit ExpressionElement(const char* src);
  ExpressionElement(const ExpressionElement& rhs);
  virtual ~ExpressionElement();

  // Returns an ExpressionPiece representing this expression.
  const ExpressionPiece& ParsedExpression() const;

  // Overridden from BytecodeElement:
  virtual void PrintSourceRepresentation(RLMachine* machine,
                                         std::ostream& oss) const final;
  virtual const size_t GetBytecodeLength() const final;
  virtual void RunOnMachine(RLMachine& machine) const final;

 private:
  int length_;

  // Storage for the parsed expression so we only have to calculate
  // it once (and so we can return it by const reference)
  ExpressionPiece parsed_expression_;
};
  
}

#endif
