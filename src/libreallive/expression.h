// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006 Peter Jolly
// Copyright (c) 2007 Elliot Glaysher
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

#ifndef SRC_LIBREALLIVE_EXPRESSION_H_
#define SRC_LIBREALLIVE_EXPRESSION_H_

#include <memory>
#include <string>
#include <vector>

#include "libreallive/alldefs.h"
#include "machine/reference.h"

class RLMachine;

namespace libreallive {

// Size of expression functions
size_t NextToken(const char* src);
size_t NextExpression(const char* src);
size_t NextString(const char* src);
size_t NextData(const char* src);

class IExpression;
class ExpressionPiece;
using Expression = std::shared_ptr<IExpression>;

std::string EvaluatePRINT(RLMachine& machine, const std::string& in);

enum class ExpressionValueType { Integer, String };

class IExpression {
 public:
  virtual ~IExpression() {}

  virtual bool is_valid() const { return false; }

  // Capability method; returns false by default. Override when
  // ExpressionPiece subclass accesses a piece of memory.
  virtual bool IsMemoryReference() const { return false; }

  // Capability method; returns false by default. Override only in
  // classes that represent a complex parameter to the type system.
  // @see Complex_T
  virtual bool IsComplexParameter() const { return false; }

  // Capability method; returns false by default. Override only in
  // classes that represent a special parameter to the type system.
  // @see Special_T
  virtual bool IsSpecialParameter() const { return false; }

  // Returns the value type of this expression (i.e. string or
  // integer)
  virtual ExpressionValueType GetExpressionValueType() const {
    return ExpressionValueType::Integer;
  }

  // Assigns the value into the memory location represented by the
  // current expression. Not all ExpressionPieces can do this, so
  // there is a default implementation which does nothing.
  virtual void SetIntegerValue(RLMachine& machine, int rvalue) {}

  // Returns the integer value of this expression; this can either be
  // a memory access or a calculation based on some subexpressions.
  virtual int GetIntegerValue(RLMachine& machine) const {
    throw Error("IExpression::GetIntegerValue() invalid on this object");
  }

  virtual void SetStringValue(RLMachine& machine, const std::string& rvalue) {
    throw Error("IExpression::SetStringValue() invalid on this object");
  }
  virtual std::string GetStringValue(RLMachine& machine) const {
    throw Error("IExpression::GetStringValue() invalid on this object");
  }

  // I used to be able to just static cast any ExpressionPiece to a
  // MemoryReference if I wanted/needed a corresponding iterator. Haeleth's
  // rlBabel library instead uses the store register as an argument to a
  // function that takes a integer reference. So this needs to be here now.
  virtual IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const {
    throw Error(
        "IExpression::GetIntegerReferenceIterator() invalid on this object");
  }
  virtual StringReferenceIterator GetStringReferenceIterator(
      RLMachine& machine) const {
    throw Error(
        "IExpression::GetStringReferenceIterator() invalid on this object");
  }

  // A persistable version of this value. This method should return RealLive
  // bytecode equal to this ExpressionPiece with all references returned.
  virtual std::string GetSerializedExpression(RLMachine& machine) const {
    throw Error(
        "IExpression::GetSerializedExpression() invalid on this object");
  }

  // A printable representation of the expression itself. Used to dump our
  // parsing of the bytecode to the console.
  virtual std::string GetDebugString() const { return "<invalid>"; }

  // In the case of Complex and Special types, adds an expression piece to the
  // list.
  virtual void AddContainedPiece(Expression piece);

  virtual const std::vector<Expression>& GetContainedPieces() const {
    throw Error("Request to GetContainedPiece() invalid!");
  }

  virtual int GetOverloadTag() const {
    throw Error("Request to GetOverloadTag() invalid!");
  }
};

class ExpressionFactory {
 public:
  static Expression StoreRegister();
  static Expression IntConstant(const int& constant);
  static Expression StrConstant(const std::string& constant);
  static Expression MemoryReference(const int& type, Expression location);
  static Expression UniaryExpression(const char operation, Expression operand);
  static Expression BinaryExpression(const char operation,
                                     Expression lhs,
                                     Expression rhs);
  static Expression ComplexExpression();
  static Expression SpecialExpression(const int tag);
};

typedef std::vector<libreallive::Expression> ExpressionPiecesVector;

}  // namespace libreallive

#endif  // SRC_LIBREALLIVE_EXPRESSION_H_
