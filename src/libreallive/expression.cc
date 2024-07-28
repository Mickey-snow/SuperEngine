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

#include "libreallive/expression.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "libreallive/alldefs.h"
#include "libreallive/intmemref.h"
#include "machine/reference.h"
#include "machine/rlmachine.h"

#include <boost/algorithm/string.hpp>

namespace {

std::string IntToBytecode(int val) {
  std::string prefix("$\xFF");
  libreallive::append_i32(prefix, val);
  return prefix;
}

bool IsUnescapedQuotationMark(const char* src, const char* current) {
  if (src == current)
    return *current == '"';
  else
    return *current == '"' && *(current - 1) != '\\';
}

}  // namespace

namespace libreallive {

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

// ----------------------------------------------------------------------
// IExpression
// ----------------------------------------------------------------------

void IExpression::AddContainedPiece(Expression piece) {
  throw Error("Request to AddContainedPiece() invalid!");
}

// ----------------------------------------------------------------------
// Store Register
// ----------------------------------------------------------------------

class StoreRegisterEx : public IExpression {
 public:
  StoreRegisterEx() = default;

  bool is_valid() const override { return true; }

  bool IsMemoryReference() const override { return true; }

  void SetIntegerValue(RLMachine& machine, int rvalue) override {
    machine.set_store_register(rvalue);
  }

  int GetIntegerValue(RLMachine& machine) const override {
    return machine.store_register();
  }

  std::string GetDebugString() const override { return "<store>"; }

  IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const override {
    return IntReferenceIterator(machine.store_register_address());
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(machine.store_register());
  }
};

// ----------------------------------------------------------------------
// Int Constant
// ----------------------------------------------------------------------

class IntConstantEx : public IExpression {
 public:
  IntConstantEx(int value) : value_(value) {}

  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override { return value_; }
  int GetIntegerValue() const { return value_; }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(value_);
  }

  std::string GetDebugString() const override { return std::to_string(value_); }

 private:
  int value_;
};

// ----------------------------------------------------------------------
// String Constant
// ----------------------------------------------------------------------

class StringConstantEx : public IExpression {
 public:
  StringConstantEx(const std::string& value) : value_(value) {}

  bool is_valid() const override { return true; }

  ExpressionValueType GetExpressionValueType() const override {
    return ExpressionValueType::String;
  }

  std::string GetStringValue(RLMachine& machine) const override {
    return value_;
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return "\"" + value_ + "\"";
  }

  std::string GetDebugString() const override { return "\"" + value_ + "\""; }

 private:
  std::string value_;
};

// ----------------------------------------------------------------------
// Memory Reference
// ----------------------------------------------------------------------

std::string GetMemoryDebugString(int type, const std::string& location) {
  std::ostringstream ret;
  if (type == STRS_LOCATION) {
    ret << "strS[";
  } else if (type == STRK_LOCATION) {
    ret << "strK[";
  } else if (type == STRM_LOCATION) {
    ret << "strM[";
  } else if (type == INTZ_LOCATION_IN_BYTECODE) {
    ret << "intZ[";
  } else if (type == INTL_LOCATION_IN_BYTECODE) {
    ret << "intL[";
  } else {
    char bank = 'A' + (type % 26);
    ret << "int" << bank << "[";
  }

  ret << location;
  ret << "]";
  return ret.str();
}

class MemoryReferenceEx : public IExpression {
 public:
  MemoryReferenceEx(int type, Expression location)
      : type_(type), location_(location) {}

  bool is_valid() const override { return true; }

  bool IsMemoryReference() const override { return true; }

  ExpressionValueType GetExpressionValueType() const override {
    return is_string_location(type_) ? ExpressionValueType::String
                                     : ExpressionValueType::Integer;
  }

  void SetIntegerValue(RLMachine& machine, int rvalue) override {
    machine.SetIntValue(IntMemRef(type_, location_->GetIntegerValue(machine)),
                        rvalue);
  }

  int GetIntegerValue(RLMachine& machine) const override {
    return machine.GetIntValue(
        IntMemRef(type_, location_->GetIntegerValue(machine)));
  }

  void SetStringValue(RLMachine& machine, const std::string& rval) override {
    machine.SetStringValue(type_, location_->GetIntegerValue(machine), rval);
  }

  std::string GetStringValue(RLMachine& machine) const override {
    return machine.GetStringValue(type_, location_->GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return GetMemoryDebugString(type_, location_->GetDebugString());
  }

  IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const override {
    if (is_string_location(type_))
      throw Error(
          "Request to GetIntegerReferenceIterator() on a string reference!");

    return IntReferenceIterator(&machine.memory(), type_,
                                location_->GetIntegerValue(machine));
  }

  StringReferenceIterator GetStringReferenceIterator(
      RLMachine& machine) const override {
    if (!is_string_location(type_))
      throw Error(
          "Request to GetStringReferenceIterator() on a string reference!");

    return StringReferenceIterator(&machine.memory(), type_,
                                   location_->GetIntegerValue(machine));
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    if (is_string_location(type_))
      return "\"" + GetStringValue(machine) + "\"";
    else
      return IntToBytecode(GetIntegerValue(machine));
  }

 private:
  int type_;
  Expression location_;
};

// ----------------------------------------------------------------------
// Simple Memory Reference
// ----------------------------------------------------------------------

class SimpleMemRefEx : public IExpression {
 public:
  SimpleMemRefEx(const int& type, const int& value)
      : type_(type), location_(value) {}

  bool is_valid() const override { return true; }

  bool IsMemoryReference() const override { return true; }

  ExpressionValueType GetExpressionValueType() const override {
    return is_string_location(type_) ? ExpressionValueType::String
                                     : ExpressionValueType::Integer;
  }

  void SetIntegerValue(RLMachine& machine, int rvalue) override {
    machine.SetIntValue(IntMemRef(type_, location_), rvalue);
  }

  int GetIntegerValue(RLMachine& machine) const override {
    return machine.GetIntValue(IntMemRef(type_, location_));
  }

  void SetStringValue(RLMachine& machine, const std::string& rval) override {
    machine.SetStringValue(type_, location_, rval);
  }

  std::string GetStringValue(RLMachine& machine) const override {
    return machine.GetStringValue(type_, location_);
  }

  IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const override {
    if (is_string_location(type_))
      throw Error(
          "Request to GetIntegerReferenceIterator() on a string reference!");

    return IntReferenceIterator(&machine.memory(), type_, location_);
  }

  StringReferenceIterator GetStringReferenceIterator(
      RLMachine& machine) const override {
    if (!is_string_location(type_))
      throw Error(
          "Request to GetStringReferenceIterator() on a string reference!");

    return StringReferenceIterator(&machine.memory(), type_, location_);
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    if (is_string_location(type_))
      return "\"" + GetStringValue(machine) + "\"";
    else
      return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return GetMemoryDebugString(type_, std::to_string(location_));
  }

 private:
  int type_;
  int location_;

  friend class ExpressionFactory;
};

Expression ExpressionFactory::MemoryReference(const int& type, Expression loc) {
  if (auto location = std::dynamic_pointer_cast<IntConstantEx>(loc))
    return std::make_shared<SimpleMemRefEx>(type, location->GetIntegerValue());
  else
    return std::make_shared<MemoryReferenceEx>(type, loc);
}

// ----------------------------------------------------------------------
// Binary Expression
// ----------------------------------------------------------------------
std::string GetBinaryDebugString(char operation,
                                 const std::string& lhs,
                                 const std::string& rhs) {
  std::ostringstream str;
  str << lhs;
  str << " ";
  switch (operation) {
    case 0:
    case 20:
      str << "+";
      break;
    case 1:
    case 21:
      str << "-";
      break;
    case 2:
    case 22:
      str << "*";
      break;
    case 3:
    case 23:
      str << "/";
      break;
    case 4:
    case 24:
      str << "%";
      break;
    case 5:
    case 25:
      str << "&";
      break;
    case 6:
    case 26:
      str << "|";
      break;
    case 7:
    case 27:
      str << "^";
      break;
    case 8:
    case 28:
      str << "<<";
      break;
    case 9:
    case 29:
      str << ">>";
      break;
    case 30:
      str << "=";
      break;
    case 40:
      str << "==";
      break;
    case 41:
      str << "!=";
      break;
    case 42:
      str << "<=";
      break;
    case 43:
      str << "< ";
      break;
    case 44:
      str << ">=";
      break;
    case 45:
      str << "> ";
      break;
    case 60:
      str << "&&";
      break;
    case 61:
      str << "||";
      break;
    default: {
      std::ostringstream ss;
      ss << "Invalid operator " << static_cast<int>(operation)
         << " in expression!";
      throw Error(ss.str());
    }
  }

  if (operation >= 0x14 && operation != 30)
    str << "=";

  str << " ";
  str << rhs;

  return str.str();
}

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

class BinaryExpressionEx : public IExpression {
 public:
  BinaryExpressionEx(char operation, Expression left, Expression right)
      : operation_(operation), left_(left), right_(right) {}

  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override {
    if (operation_ >= 20 && operation_ < 30) {
      int value =
          PerformBinaryOperationOn(operation_, left_->GetIntegerValue(machine),
                                   right_->GetIntegerValue(machine));
      left_->SetIntegerValue(machine, value);
      return value;
    } else if (operation_ == 30) {
      int value = right_->GetIntegerValue(machine);
      left_->SetIntegerValue(machine, value);
      return value;
    } else {
      return PerformBinaryOperationOn(operation_,
                                      left_->GetIntegerValue(machine),
                                      right_->GetIntegerValue(machine));
    }
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return GetBinaryDebugString(operation_, left_->GetDebugString(),
                                right_->GetDebugString());
  }

 private:
  char operation_;
  Expression left_;
  Expression right_;
};

// ----------------------------------------------------------------------
// Uniary Expression
// ----------------------------------------------------------------------

class UniaryEx : public IExpression {
 public:
  UniaryEx(const char& op, const Expression ex)
      : operation_(op), operand_(ex) {}

  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override {
    return PerformUniaryOperationOn(operand_->GetIntegerValue(machine));
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    std::ostringstream str;
    if (operation_ == 0x01) {
      str << "-";
    }
    str << operand_->GetDebugString();

    return str.str();
  }

 private:
  char operation_;
  Expression operand_;

  int PerformUniaryOperationOn(int int_operand) const {
    int result = int_operand;
    switch (operation_) {
      case 0x01:
        result = -int_operand;
        break;
      default:
        break;
    }
    return result;
  }
};

Expression ExpressionFactory::UniaryExpression(const char operation,
                                               Expression operand) {
  return std::make_shared<UniaryEx>(operation, operand);
}

// ----------------------------------------------------------------------
// Simple Assignment
// ----------------------------------------------------------------------

class SimpleAssignEx : public IExpression {
 public:
  SimpleAssignEx(const int& type, const int& loc, const int& val)
      : type_(type), location_(loc), value_(val) {}
  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override {
    machine.SetIntValue(IntMemRef(type_, location_), value_);
    return value_;
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return GetBinaryDebugString(
        30, GetMemoryDebugString(type_, std::to_string(location_)),
        std::to_string(value_));
  }

 private:
  int type_;
  int location_;
  int value_;

  friend class ExpressionFactory;
};

// ----------------------------------------------------------------------
// Complex Expression
// ----------------------------------------------------------------------

class ComplexEx : public IExpression {
 public:
  ComplexEx() {}

  bool is_valid() const override { return true; }

  bool IsComplexParameter() const override { return true; }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    std::string s = "(";
    for (const auto& it : expression_)
      s += '(' + it->GetSerializedExpression(machine) + ')';
    s += ')';
    return s;
  }

  std::string GetDebugString() const override {
    string s = "(";
    for (auto const& piece : expression_) {
      s += piece->GetDebugString();
      if(piece != expression_.back())
        s += ", ";
    }
    s += ")";
    return s;
  }

  void AddContainedPiece(Expression piece) override {
    expression_.push_back(piece);
  }

  const std::vector<Expression>& GetContainedPieces() const override {
    return expression_;
  }

 private:
  std::vector<Expression> expression_;
};

// ----------------------------------------------------------------------
// Special Expression
// ----------------------------------------------------------------------

class SpecialEx : public IExpression {
 public:
  SpecialEx(const int& tag) : overload_tag_(tag) {}

  bool is_valid() const override { return true; }

  bool IsSpecialParameter() const override { return true; }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    std::string s = "a";
    s += char(overload_tag_);

    if (expression_.size() > 1)
      s.append("(");
    for (auto const& piece : expression_) {
      s += piece->GetSerializedExpression(machine);
    }
    if (expression_.size() > 1)
      s.append(")");
    return s;
  }

  std::string GetDebugString() const override {
    std::ostringstream oss;

    oss << int(overload_tag_) << ":{";

    bool first = true;
    for (auto const& piece : expression_) {
      if (!first) {
        oss << ", ";
      } else {
        first = false;
      }

      oss << piece->GetDebugString();
    }
    oss << "}";

    return oss.str();
  }

  void AddContainedPiece(Expression piece) override {
    expression_.push_back(piece);
  }

  const std::vector<Expression>& GetContainedPieces() const override {
    return expression_;
  }

  int GetOverloadTag() const override { return overload_tag_; }

 private:
  int overload_tag_;
  std::vector<Expression> expression_;
};

// ----------------------------------------------------------------------
// static
Expression ExpressionFactory::StoreRegister() {
  return std::make_shared<StoreRegisterEx>();
}

// static
Expression ExpressionFactory::IntConstant(const int& constant) {
  return std::make_shared<IntConstantEx>(constant);
}

// static
Expression ExpressionFactory::StrConstant(const std::string& constant) {
  return std::make_shared<StringConstantEx>(constant);
}

// static
Expression ExpressionFactory::BinaryExpression(const char operation,
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

// static
Expression ExpressionFactory::ComplexExpression() {
  return std::make_shared<ComplexEx>();
}

// static
Expression ExpressionFactory::SpecialExpression(const int tag) {
  return std::make_shared<SpecialEx>(tag);
}

}  // namespace libreallive
