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

#include "libreallive/elements/expression.hpp"
#include "machine/rlmachine.hpp"

namespace libreallive {

// -----------------------------------------------------------------------
// ExpressionElement
// -----------------------------------------------------------------------

ExpressionElement::ExpressionElement(Expression expr)
    : length_(0), parsed_expression_(expr) {}

ExpressionElement::ExpressionElement(const int& len, Expression expr)
    : length_(len), parsed_expression_(expr) {}

ExpressionElement::~ExpressionElement() {}

Expression ExpressionElement::ParsedExpression() const {
  return parsed_expression_;
}

std::string ExpressionElement::GetSourceRepresentation(IModuleManager*) const {
  return ParsedExpression()->GetDebugString();
}

const size_t ExpressionElement::GetBytecodeLength() const { return length_; }

void ExpressionElement::RunOnMachine(RLMachine& machine) const {
  machine.ExecuteExpression(*this);
}

}  // namespace libreallive
