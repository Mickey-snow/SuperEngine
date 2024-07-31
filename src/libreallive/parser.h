// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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

#ifndef SRC_LIBREALLIVE_PARSER_H_
#define SRC_LIBREALLIVE_PARSER_H_

#include <ostream>
#include <string>
#include <vector>

#include "libreallive/alldefs.h"
#include "libreallive/elements/bytecode.h"
#include "libreallive/elements/comma.h"
#include "libreallive/elements/command.h"
#include "libreallive/elements/expression.h"
#include "libreallive/elements/meta.h"
#include "libreallive/elements/textout.h"

namespace libreallive {

// Converts a parameter string (as read from the binary SEEN.TXT file)
// into a human readable (and printable) format.
std::string ParsableToPrintableString(const std::string& src);

// Converts a printable string (i.e., "$ 05 [ $ FF EE 03 00 00 ]")
// into one that can be parsed by all the get_expr family of functions.
std::string PrintableToParsableString(const std::string& src);

void PrintParameterString(std::ostream& oss,
                          const std::vector<std::string>& paramseters);

class Parser {
 public:
  explicit Parser();
  explicit Parser(std::shared_ptr<ConstructionData> cdata);

  BytecodeElement* ParseBytecode(const char* stream, const char* end);

  BytecodeElement* ParseBytecode(const std::string& src);

  TextoutElement* ParseTextout(const char* stream, const char* end);
  
  CommandElement* ParseCommand(const char* stream);
  
  static CommandElement* BuildFunctionElement(const char* stream);

 private:
  std::shared_ptr<ConstructionData> cdata;
  char entrypoint_marker;
};

class Factory {
 public:
  static CommandElement* MakeFunction(const char* opcode,
                                      const std::vector<std::string>& params);

  static ExpressionElement* MakeExpression(const char* stream);

  static MetaElement* MakeMeta(std::shared_ptr<ConstructionData> cdata,
                               const char* stream);
};

class ExpressionParser {
 public:
  // token -> 0xff int32 <intConst>
  // | 0xc8 <StoreReg>
  // | type [ expr ] <MemoryRef>
  static Expression GetExpressionToken(const char*& src);
  // term -> $ token
  // | \ 0x00 term
  // | \ 0x01 uniary
  // | ( boolean )
  static Expression GetExpressionTerm(const char*& src);
  // arithmatic -> term ( op term )*
  static Expression GetExpressionArithmatic(const char*& src);
  // cond -> arithmatic ( op arithmatic )*
  static Expression GetExpressionCondition(const char*& src);
  // boolean -> cond ( op cond )*
  static Expression GetExpressionBoolean(const char*& src);
  // expr -> boolean
  static Expression GetExpression(const char*& src);
  // assign -> term op expr
  static Expression GetAssignment(const char*& src);
  // data -> , data
  // | \n . . data
  // | string
  // | tag complexparam
  // | expr
  static Expression GetData(const char*& src);
  // complexparam -> , data
  // | expr
  // | ( data+ )
  static Expression GetComplexParam(const char*& src);

private:
  // Left recursion removed
  static Expression GetExpressionArithmaticLoop(const char*& src, Expression toc);
  static Expression GetExpressionArithmaticLoopHiPrec(const char*& src, Expression tok);
  static Expression GetExpressionConditionLoop(const char*& src, Expression tok);
  static Expression GetExpressionBooleanLoopAnd(const char*& src, Expression tok);
  static Expression GetExpressionBooleanLoopOr(const char*& src, Expression tok);
  static Expression GetString(const char*& src);
};

}  // namespace libreallive

#endif  // SRC_LIBREALLIVE_BYTECODE_H_
