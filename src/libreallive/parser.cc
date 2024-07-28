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

#include "libreallive/parser.h"
#include "libreallive/expression.h"
#include "libreallive/scenario.h"
#include "machine/rlmachine.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>

namespace libreallive {

std::string ParsableToPrintableString(const std::string& src) {
  string output;

  bool firstToken = true;
  for (char tok : src) {
    if (firstToken) {
      firstToken = false;
    } else {
      output += " ";
    }

    if (tok == '(' || tok == ')' || tok == '$' || tok == '[' || tok == ']') {
      output.push_back(tok);
    } else {
      std::ostringstream ss;
      ss << std::hex << std::setw(2) << std::setfill('0') << int(tok);
      output += ss.str();
    }
  }

  return output;
}

std::string PrintableToParsableString(const std::string& src) {
  typedef boost::tokenizer<boost::char_separator<char>> ttokenizer;

  std::string output;

  boost::char_separator<char> sep(" ");
  ttokenizer tokens(src, sep);
  for (string const& tok : tokens) {
    if (tok.size() > 2)
      throw libreallive::Error(
          "Invalid string given to printableToParsableString");

    if (tok == "(" || tok == ")" || tok == "$" || tok == "[" || tok == "]") {
      output.push_back(tok[0]);
    } else {
      int charToAdd;
      std::istringstream ss(tok);
      ss >> std::hex >> charToAdd;
      output.push_back((char)charToAdd);
    }
  }

  return output;
}

void PrintParameterString(std::ostream& oss,
                          const std::vector<std::string>& parameters) {
  bool first = true;
  oss << "(";
  for (std::string const& param : parameters) {
    if (!first) {
      oss << ", ";
    }
    first = false;

    // Take the binary stuff and try to get usefull, printable values.
    const char* start = param.c_str();
    try {
      Expression piece(ExpressionParser::GetData(start));
      oss << piece->GetDebugString();
    } catch (libreallive::Error& e) {
      // Any error throw here is a parse error.
      oss << "{RAW : " << ParsableToPrintableString(param) << "}";
    }
  }
  oss << ")";
}

// -----------------------------------------------------------------------
// class Parser
// -----------------------------------------------------------------------
Parser::Parser() : cdata(std::make_shared<ConstructionData>()) {}

Parser::Parser(std::shared_ptr<ConstructionData> data) : cdata(data) {}

char entrypoint_marker = '@';

BytecodeElement* Parser::ParseBytecode(const std::string& src) {
  return ParseBytecode(src.c_str(), src.c_str() + src.size());
}

BytecodeElement* Parser::ParseBytecode(const char* stream, const char* end) {
  const char c = *stream;
  if (c == '!')
    entrypoint_marker = '!';

  BytecodeElement* result = nullptr;
  switch (c) {
    case 0:
    case ',':
      result = new CommaElement;
      break;
    case '\n':
      result = new MetaElement(nullptr, stream);
      break;
    case '@':
    case '!':
      result = new MetaElement(cdata, stream);
      break;
    case '$':
      result = Factory::MakeExpression(stream);
      break;
    case '#':
      result = ParseFunction(stream);
      break;
    default:
      result = ParseTextout(stream, end);
      break;
  }

  // result->PrintSourceRepresentation(nullptr, std::cout);
  // std::string rawbytes(stream, result->GetBytecodeLength());
  // std::cout<<ParsableToPrintableString(rawbytes)<<std::endl<<std::endl;
  return result;
}

TextoutElement* Parser::ParseTextout(const char* src, const char* file_end) {
  const char* end = src;
  bool quoted = false;
  while (end < file_end) {
    if (quoted) {
      quoted = *end != '"';
      if (*end == '\\' && end[1] == '"')  // escaped quote
        ++end;
    } else {
      if (*end == ',')  // not a comma element
        ++end;
      quoted = *end == '"';

      // new element
      if (!*end || *end == '#' || *end == '$' || *end == '\n' || *end == '@' ||
          *end == entrypoint_marker)
        break;
    }

    if ((*end >= 0x81 && *end <= 0x9f) || (*end >= 0xe0 && *end <= 0xef))
      end += 2;  // shift.jis
    else
      ++end;
  }

  return new TextoutElement(src, end);
}

CommandElement* Parser::ParseFunction(const char* stream) {
  // opcode: 0xttmmoooo (Type, Module, Opcode: e.g. 0x01030101 = 1:03:00257
  const unsigned long opcode =
      (stream[1] << 24) | (stream[2] << 16) | (stream[4] << 8) | stream[3];
  switch (opcode) {
    case 0x00010000:
    case 0x00010005:
    case 0x00050001:
    case 0x00050005:
    case 0x00060001:
    case 0x00060005:
      return new GotoElement(stream, *cdata);
    case 0x00010001:
    case 0x00010002:
    case 0x00010006:
    case 0x00010007:
    case 0x00050002:
    case 0x00050006:
    case 0x00050007:
    case 0x00060000:
    case 0x00060002:
    case 0x00060006:
    case 0x00060007:
      return new GotoIfElement(stream, *cdata);
    case 0x00010003:
    case 0x00010008:
    case 0x00050003:
    case 0x00050008:
    case 0x00060003:
    case 0x00060008:
      return new GotoOnElement(stream, *cdata);
    case 0x00010004:
    case 0x00010009:
    case 0x00050004:
    case 0x00050009:
    case 0x00060004:
    case 0x00060009:
      return new GotoCaseElement(stream, *cdata);
    case 0x00010010:
    case 0x00060010:
      return new GosubWithElement(stream, *cdata);

      // Select elements.
    case 0x00020000:
    case 0x00020001:
    case 0x00020002:
    case 0x00020003:
    case 0x00020010:
      return new SelectElement(stream);
  }
  // default:
  return BuildFunctionElement(stream);
}

CommandElement* Parser::BuildFunctionElement(const char* stream) {
  const char* ptr = stream;
  ptr += 8;
  std::vector<std::string> params;
  if (*ptr == '(') {
    const char* end = ptr + 1;
    while (*end != ')') {
      const size_t len = NextData(end);
      params.emplace_back(end, len);
      end += len;
    }
  }

  return Factory::MakeFunction(stream, params);
}

// -----------------------------------------------------------------------
// Factory
// -----------------------------------------------------------------------

CommandElement* Factory::MakeFunction(const char* opcode,
                                      const std::vector<std::string>& params) {
  if (params.size() == 0)
    return new VoidFunctionElement(opcode);
  else if (params.size() == 1)
    return new SingleArgFunctionElement(opcode, params.front());
  else
    return new FunctionElement(opcode, params);
}

ExpressionElement* Factory::MakeExpression(const char* stream) {
  const char* end = stream;
  auto expr = ExpressionParser::GetAssignment(end);
  auto len = std::distance(stream, end);
  return new ExpressionElement(len, expr);
}

// -----------------------------------------------------------------------
// ExpressionParser
// -----------------------------------------------------------------------
//
// Functions used at runtime to parse expressions, both as
// ExpressionPieces, parameters in function calls, and other uses in
// some special cases. These functions form a recursive descent parser
// that parses expressions and parameters in Reallive byte code into
// ExpressionPieces, which are executed with the current RLMachine.
//
// These functions were translated from the O'Caml implementation in
// dissassembler.ml in RLDev, so really, while I coded this, Haeleth
// really gets all the credit.
// -----------------------------------------------------------------------

Expression ExpressionParser::GetExpressionToken(const char*& src) {
  if (src[0] == 0xff) {
    src++;
    int value = read_i32(src);
    src += 4;
    return ExpressionFactory::IntConstant(value);
  } else if (src[0] == 0xc8) {
    src++;
    return ExpressionFactory::StoreRegister();
  } else if ((src[0] != 0xc8 && src[0] != 0xff) && src[1] == '[') {
    int type = src[0];
    src += 2;
    Expression location = GetExpression(src);

    if (src[0] != ']') {
      std::ostringstream ss;
      ss << "Unexpected character '" << src[0] << "' in GetExpressionToken"
         << " (']' expected)";
      throw Error(ss.str());
    }
    src++;

    return ExpressionFactory::MemoryReference(type, location);
  } else if (src[0] == 0) {
    throw Error("Unexpected end of buffer in GetExpressionToken");
  } else {
    std::ostringstream err;
    err << "Unknown toke type 0x" << std::hex << (short)src[0]
        << " in GetExpressionToken" << std::endl;
    throw Error(err.str());
  }
}

Expression ExpressionParser::GetExpressionTerm(const char*& src) {
  if (src[0] == '$') {
    src++;
    return GetExpressionToken(src);
  } else if (src[0] == '\\' && src[1] == 0x00) {
    src += 2;
    return GetExpressionTerm(src);
  } else if (src[0] == '\\' && src[1] == 0x01) {
    // Uniary -
    src += 2;
    return ExpressionFactory::UniaryExpression(0x01, GetExpressionTerm(src));
  } else if (src[0] == '(') {
    src++;
    Expression p = GetExpressionBoolean(src);
    if (src[0] != ')') {
      std::ostringstream ss;
      ss << "Unexpected character '" << src[0] << "' in GetExpressionTerm"
         << " (')' expected)";
      throw Error(ss.str());
    }
    src++;
    return p;
  } else if (src[0] == 0) {
    throw Error("Unexpected end of buffer in GetExpressionTerm");
  } else {
    std::ostringstream err;
    err << "Unknown token type 0x" << std::hex << (short)src[0]
        << " in GetExpressionTerm";
    throw Error(err.str());
  }
}

Expression ExpressionParser::GetExpressionArithmaticLoopHiPrec(const char*& src, Expression tok) {
  if (src[0] == '\\' && src[1] >= 0x02 && src[1] <= 0x09) {
    char op = src[1];
    // Advance past this operator
    src += 2;
    Expression new_piece = ExpressionFactory::BinaryExpression(
        op, tok, ExpressionParser::GetExpressionTerm(src));
    return GetExpressionArithmaticLoopHiPrec(src, new_piece);
  } else {
    // We don't consume anything and just return our input token.
    return tok;
  }
}

Expression ExpressionParser::GetExpressionArithmaticLoop(const char*& src, Expression tok) {
  if (src[0] == '\\' && (src[1] == 0x00 || src[1] == 0x01)) {
    char op = src[1];
    src += 2;
    Expression other = GetExpressionTerm(src);
    Expression rhs = GetExpressionArithmaticLoopHiPrec(src, other);
    Expression new_piece = ExpressionFactory::BinaryExpression(op, tok, rhs);
    return GetExpressionArithmaticLoop(src, new_piece);
  } else {
    return tok;
  }
}

Expression ExpressionParser::GetExpressionArithmatic(const char*& src) {
  return GetExpressionArithmaticLoop(
      src, GetExpressionArithmaticLoopHiPrec(src, GetExpressionTerm(src)));
}

Expression ExpressionParser::GetExpressionConditionLoop(const char*& src, Expression tok) {
  if (src[0] == '\\' && (src[1] >= 0x28 && src[1] <= 0x2d)) {
    char op = src[1];
    src += 2;
    Expression rhs = GetExpressionArithmatic(src);
    Expression new_piece = ExpressionFactory::BinaryExpression(op, tok, rhs);
    return GetExpressionConditionLoop(src, new_piece);
  } else {
    return tok;
  }
}

Expression ExpressionParser::GetExpressionCondition(const char*& src) {
  return GetExpressionConditionLoop(src, GetExpressionArithmatic(src));
}

Expression ExpressionParser::GetExpressionBooleanLoopAnd(const char*& src, Expression tok) {
  if (src[0] == '\\' && src[1] == '<') {
    src += 2;
    Expression rhs = GetExpressionCondition(src);
    return GetExpressionBooleanLoopAnd(
        src, ExpressionFactory::BinaryExpression(0x3c, tok, rhs));
  } else {
    return tok;
  }
}

Expression ExpressionParser::GetExpressionBooleanLoopOr(const char*& src, Expression tok) {
  if (src[0] == '\\' && src[1] == '=') {
    src += 2;
    Expression innerTerm = GetExpressionCondition(src);
    Expression rhs = GetExpressionBooleanLoopAnd(src, innerTerm);
    return GetExpressionBooleanLoopOr(
        src, ExpressionFactory::BinaryExpression(0x3d, tok, rhs));
  } else {
    return tok;
  }
}

Expression ExpressionParser::GetExpressionBoolean(const char*& src) {
  return GetExpressionBooleanLoopOr(
      src, GetExpressionBooleanLoopAnd(src, GetExpressionCondition(src)));
}

Expression ExpressionParser::GetExpression(const char*& src) {
  return GetExpressionBoolean(src);
}

// Parses an expression of the form [dest] = [source expression];
Expression ExpressionParser::GetAssignment(const char*& src) {
  Expression itok(GetExpressionTerm(src));
  int op = src[1];
  src += 2;
  Expression etok(GetExpression(src));
  if (op >= 0x14 && op <= 0x24) {
    return ExpressionFactory::BinaryExpression(op, itok, etok);
  } else {
    throw Error("Undefined assignment in GetAssignment");
  }
}

// Parses a string in the parameter list.
Expression ExpressionParser::GetString(const char*& src) {
  // Get the length of this string in the bytecode:
  size_t length = NextString(src);

  string s;
  // Check to see if the string is quoted;
  if (src[0] == '"')
    s = string(src + 1, src + length - 1);
  else
    s = string(src, src + length);

  // Increment the source by that many characters
  src += length;

  // Unquote the internal quotations.
  boost::replace_all(s, "\\\"", "\"");

  return ExpressionFactory::StrConstant(s);
}

// Parses a parameter in the parameter list. This is the only method
// of all the get_*(const char*& src) functions that can parse
// strings. It also deals with things like special and complex
// parameters.
Expression ExpressionParser::GetData(const char*& src) {
  if (*src == ',') {
    ++src;
    return GetData(src);
  } else if (*src == '\n') {
    src += 3;
    return GetData(src);
  } else if ((*src >= 0x81 && *src <= 0x9f) || (*src >= 0xe0 && *src <= 0xef) ||
             (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') ||
             *src == ' ' || *src == '?' || *src == '_' || *src == '"' ||
             strcmp(src, "###PRINT(") == 0) {
    return GetString(src);
  } else if (*src == 'a') {
    // TODO(erg): Cleanup below.
    const char*& end = src;

    Expression cep = ExpressionFactory::ComplexExpression();

    if (*end++ == 'a') {
      int tag = *end++;

      // Some special cases have multiple tags.
      if (*end == 'a') {
        end++;
        int second = *end++;
        tag = (second << 16) | tag;
      }

      cep = ExpressionFactory::SpecialExpression(tag);

      if (*end != '(') {
        // We have a single parameter in this special expression;
        cep->AddContainedPiece(GetData(end));
        return cep;
      } else {
        end++;
      }
    } else {
      cep = ExpressionFactory::ComplexExpression();
    }

    while (*end != ')') {
      cep->AddContainedPiece(GetData(end));
    }

    return cep;
  } else {
    return GetExpression(src);
  }
}

Expression ExpressionParser::GetComplexParam(const char*& src) {
  if (*src == ',') {
    ++src;
    return GetData(src);
  } else if (*src == '(') {
    ++src;
    Expression cep = ExpressionFactory::ComplexExpression();

    while (*src != ')')
      cep->AddContainedPiece(GetData(src));

    return cep;
  } else {
    return GetExpression(src);
  }
}

}  // namespace libreallive
