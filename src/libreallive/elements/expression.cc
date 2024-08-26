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

#include "libreallive/elements/expression.h"
#include "machine/rlmachine.h"

namespace libreallive{
// -----------------------------------------------------------------------
// ExpressionElement
// -----------------------------------------------------------------------

  ExpressionElement::ExpressionElement(const char* src){
  const char* end = src;
  parsed_expression_ = GetAssignment(end);
  length_ = std::distance(src, end);
}

ExpressionElement::ExpressionElement(const long val)
    : length_(0),
      parsed_expression_(ExpressionFactory::IntConstant(val)) {
}

ExpressionElement::ExpressionElement(const ExpressionElement& rhs)
    : length_(0),
      parsed_expression_(rhs.parsed_expression_) {
}

ExpressionElement::~ExpressionElement() {}

Expression ExpressionElement::ParsedExpression() const {
  return parsed_expression_;
}

void ExpressionElement::PrintSourceRepresentation(RLMachine* machine,
                                                  std::ostream& oss) const {
  oss << ParsedExpression()->GetDebugString() << std::endl;
}

const size_t ExpressionElement::GetBytecodeLength() const {
  return length_;
}

void ExpressionElement::RunOnMachine(RLMachine& machine) const {
  machine.ExecuteExpression(*this);
}
}