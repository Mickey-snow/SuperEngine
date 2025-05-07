// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
// Copyright (c) 2007 Elliot Glaysher
// Copyright (c) 2024 Serina Sakurai
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

#include "libreallive/expression.hpp"

#include "libreallive/expression_visitor.hpp"
#include "libreallive/parser.hpp"

#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string>

namespace libreallive {
namespace {
bool IsUnescapedQuotationMark(const char* src, const char* current) {
  if (src == current)
    return *current == '"';
  else
    return *current == '"' && *(current - 1) != '\\';
}
}  // namespace

std::string GetBankName(int type) {
  if (is_string_location(type)) {
    StrMemoryLocation dummy(type, 0);
    return ToString(dummy.Bank());
  } else {
    IntMemoryLocation dummy(IntMemRef(type, 0));
    return ToString(dummy.Bank(), dummy.Bitwidth());
  }
}

// -----------------------------------------------------------------------
// enum class Op
// -----------------------------------------------------------------------

std::string ToString(Op op) {
  switch (op) {
    case Op::Add:
      return "+";
    case Op::Sub:
      return "-";
    case Op::Mul:
      return "*";
    case Op::Div:
      return "/";
    case Op::Mod:
      return "%";
    case Op::BitAnd:
      return "&";
    case Op::BitOr:
      return "|";
    case Op::BitXor:
      return "^";
    case Op::ShiftLeft:
      return "<<";
    case Op::ShiftRight:
      return ">>";
    case Op::AddAssign:
      return "+=";
    case Op::SubAssign:
      return "-=";
    case Op::MulAssign:
      return "*=";
    case Op::DivAssign:
      return "/=";
    case Op::ModAssign:
      return "%=";
    case Op::BitAndAssign:
      return "&=";
    case Op::BitOrAssign:
      return "|=";
    case Op::BitXorAssign:
      return "^=";
    case Op::ShiftLeftAssign:
      return "<<=";
    case Op::ShiftRightAssign:
      return ">>=";
    case Op::Assign:
      return "=";
    case Op::Equal:
      return "==";
    case Op::NotEqual:
      return "!=";
    case Op::LessEqual:
      return "<=";
    case Op::Less:
      return "<";
    case Op::GreaterEqual:
      return ">=";
    case Op::Greater:
      return ">";
    case Op::LogicalAnd:
      return "&&";
    case Op::LogicalOr:
      return "||";
    default:
      return "???";
  }
}

// -----------------------------------------------------------------------
// Expression Tokenization
//
// Functions that tokenize expression data while parsing the bytecode
// to create the BytecodeElements. These functions simply tokenize and
// mark boundaries; they do not perform any parsing.

size_t NextToken(const char* src) {
  if (*src++ != '$')
    return 0;
  if (*src++ == 0xff)
    return 6;
  if (*src++ != '[')
    return 2;
  return 4 + NextExpression(src);
}

static size_t NextTerm(const char* src) {
  if (*src == '(')
    return 2 + NextExpression(src + 1);
  if (*src == '\\')
    return 2 + NextTerm(src + 2);
  return NextToken(src);
}

static size_t NextArithmatic(const char* src) {
  size_t lhs = NextTerm(src);
  return (src[lhs] == '\\') ? lhs + 2 + NextArithmatic(src + lhs + 2) : lhs;
}

static size_t NextCondition(const char* src) {
  size_t lhs = NextArithmatic(src);
  return (src[lhs] == '\\' && src[lhs + 1] >= 0x28 && src[lhs + 1] <= 0x2d)
             ? lhs + 2 + NextArithmatic(src + lhs + 2)
             : lhs;
}

static size_t NextAnd(const char* src) {
  size_t lhs = NextCondition(src);
  return (src[lhs] == '\\' && src[lhs + 1] == '<')
             ? lhs + 2 + NextAnd(src + lhs + 2)
             : lhs;
}

size_t NextExpression(const char* src) {
  size_t lhs = NextAnd(src);
  return (src[lhs] == '\\' && src[lhs + 1] == '=')
             ? lhs + 2 + NextExpression(src + lhs + 2)
             : lhs;
}

size_t NextString(const char* src) {
  bool quoted = false;
  const char* end = src;

  while (true) {
    if (quoted) {
      quoted = !IsUnescapedQuotationMark(src, end);
      if (!quoted && *(end - 1) != '\\') {
        end++;  // consume the final quote
        break;
      }
    } else {
      quoted = IsUnescapedQuotationMark(src, end);
      if (strncmp(end, "###PRINT(", 9) == 0) {
        end += 9;
        end += 1 + NextExpression(end);
        continue;
      }
      if (!((*end >= 0x81 && *end <= 0x9f) || (*end >= 0xe0 && *end <= 0xef) ||
            (*end >= 'a' && *end <= 'z') || (*end >= 'A' && *end <= 'Z') ||
            (*end >= '0' && *end <= '9') || *end == ' ' || *end == '?' ||
            *end == '_' || *end == '"' || *end == '\\')) {
        break;
      }
    }
    if ((*end >= 0x81 && *end <= 0x9f) || (*end >= 0xe0 && *end <= 0xef))
      end += 2;
    else
      ++end;
  }

  if (end > src && *(end - 1) == 'a' && (*end == 0 || *end == 1))
    --end;
  return end - src;
}

size_t NextData(const char* src) {
  if (*src == ',')
    return 1 + NextData(src + 1);
  if (*src == '\n')
    return 3 + NextData(src + 3);
  if ((*src >= 0x81 && *src <= 0x9f) || (*src >= 0xe0 && *src <= 0xef) ||
      (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') ||
      *src == ' ' || *src == '?' || *src == '_' || *src == '"' ||
      strcmp(src, "###PRINT(") == 0)
    return NextString(src);
  if (*src == 'a' || *src == '(') {
    const char* end = src;
    if (*end++ == 'a') {
      ++end;

      // Some special cases have multiple tags.
      if (*end == 'a')
        end += 2;

      if (*end != '(') {
        end += NextData(end);
        return end - src;
      } else {
        end++;
      }
    }

    while (*end != ')')
      end += NextData(end);
    end++;
    if (*end == '\\')
      end += NextExpression(end);
    return end - src;
  } else {
    return NextExpression(src);
  }
}

std::string EvaluatePRINT(RLMachine& machine, const std::string& in) {
  // Currently, this doesn't evaluate the # commands inline. See 5.12.11 of the
  // rldev manual.
  if (boost::starts_with(in, "###PRINT(")) {
    const char* expression_start = in.c_str() + 9;
    Expression piece(ExpressionParser::GetExpression(expression_start));

    if (*expression_start != ')') {
      std::ostringstream ss;
      ss << "Unexpected character '" << *expression_start
         << "' in evaluatePRINT (')' expected)";
      throw Error(ss.str());
    }

    return piece->GetStringValue(machine);
  } else {
    // Just a normal string we can ignore
    return in;
  }
}

void IExpression::AddContainedPiece(Expression piece) {
  throw Error("Request to AddContainedPiece() invalid!");
}

std::string IExpression::GetDebugString() {
  return std::visit(DebugVisitor(), AsVariant());
}

// ----------------------------------------------------------------------

int PerformBinaryOperationOn(char operation, int lhs, int rhs) {
  switch (operation) {
    case 0:
    case 20:
      return lhs + rhs;
    case 1:
    case 21:
      return lhs - rhs;
    case 2:
    case 22:
      return lhs * rhs;
    case 3:
    case 23:
      return rhs != 0 ? lhs / rhs : lhs;
    case 4:
    case 24:
      return rhs != 0 ? lhs % rhs : lhs;
    case 5:
    case 25:
      return lhs & rhs;
    case 6:
    case 26:
      return lhs | rhs;
    case 7:
    case 27:
      return lhs ^ rhs;
    case 8:
    case 28:
      return lhs << rhs;
    case 9:
    case 29:
      return lhs >> rhs;
    case 40:
      return lhs == rhs;
    case 41:
      return lhs != rhs;
    case 42:
      return lhs <= rhs;
    case 43:
      return lhs < rhs;
    case 44:
      return lhs >= rhs;
    case 45:
      return lhs > rhs;
    case 60:
      return lhs && rhs;
    case 61:
      return lhs || rhs;
    default: {
      std::ostringstream ss;
      ss << "Invalid operator " << static_cast<int>(operation)
         << " in expression!";
      throw Error(ss.str());
    }
  }
}

Expression BinaryExpressionEx::Create(const char operation,
                                      Expression l,
                                      Expression r) {
  if (auto rhs = std::dynamic_pointer_cast<IntConstantEx>(r)) {
    if (auto lhs = std::dynamic_pointer_cast<IntConstantEx>(l)) {
      // We can fast path so that we just compute the integer expression here.
      int value = PerformBinaryOperationOn(operation, lhs->GetIntegerValue(),
                                           rhs->GetIntegerValue());
      return std::make_shared<IntConstantEx>(value);
    }
    if (auto lhs = std::dynamic_pointer_cast<SimpleMemRefEx>(l);
        lhs && operation == 30) {
      // We can fast path so we don't allocate memory by stashing the memory
      // reference and the value in this piece.
      int type = lhs->type_, location = lhs->location_;
      int value = rhs->GetIntegerValue();
      return std::make_shared<SimpleAssignEx>(type, location, value);
    }
  }
  return std::make_shared<BinaryExpressionEx>(operation, l, r);
}

Expression CreateMemoryReference(const int& type, Expression loc) {
  if (auto location = std::dynamic_pointer_cast<IntConstantEx>(loc))
    return std::make_shared<SimpleMemRefEx>(type, location->GetIntegerValue());
  else
    return std::make_shared<MemoryReferenceEx>(type, loc);
}

}  // namespace libreallive
