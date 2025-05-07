// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006 Peter Jolly
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

#pragma once

#include "core/memory.hpp"
#include "libreallive/alldefs.hpp"
#include "libreallive/intmemref.hpp"
#include "machine/reference.hpp"
#include "machine/rlmachine.hpp"

#include <format>
#include <memory>
#include <string>
#include <vector>

class RLMachine;

namespace libreallive {

// helper
namespace {
static inline std::string IntToBytecode(int val) {
  std::string prefix("$\xFF");
  libreallive::append_i32(prefix, val);
  return prefix;
}
static std::string GetBankName(int type) {
  using namespace libreallive;
  if (is_string_location(type)) {
    StrMemoryLocation dummy(type, 0);
    return ToString(dummy.Bank());
  } else {
    IntMemoryLocation dummy(IntMemRef(type, 0));
    return ToString(dummy.Bank(), dummy.Bitwidth());
  }
}
}  // namespace

// Size of expression functions
size_t NextToken(const char* src);
size_t NextExpression(const char* src);
size_t NextString(const char* src);
size_t NextData(const char* src);

class IExpression;
class ExpressionPiece;
using Expression = std::shared_ptr<IExpression>;
using ExpressionPiecesVector = std::vector<libreallive::Expression>;

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

enum Op : char {
  // Arithmetic Operators
  Add = 0,  // "+"
  Sub = 1,  // "-", also unary minus
  Mul = 2,  // "*"
  Div = 3,  // "/"
  Mod = 4,  // "%"

  // Bitwise Operators
  BitAnd = 5,      // "&"
  BitOr = 6,       // "|"
  BitXor = 7,      // "^"
  ShiftLeft = 8,   // "<<"
  ShiftRight = 9,  // ">>"

  // Compound Assignment Operators
  AddAssign = 20,         // "+="
  SubAssign = 21,         // "-="
  MulAssign = 22,         // "*="
  DivAssign = 23,         // "/="
  ModAssign = 24,         // "%="
  BitAndAssign = 25,      // "&="
  BitOrAssign = 26,       // "|="
  BitXorAssign = 27,      // "^="
  ShiftLeftAssign = 28,   // "<<="
  ShiftRightAssign = 29,  // ">>="

  // Assignment Operator
  Assign = 30,  // "="

  // Comparison Operators
  Equal = 40,         // "=="
  NotEqual = 41,      // "!="
  LessEqual = 42,     // "<="
  Less = 43,          // "<"
  GreaterEqual = 44,  // ">="
  Greater = 45,       // ">"

  // Logical Operators
  LogicalAnd = 60,  // "&&"
  LogicalOr = 61    // "||"
};
std::string ToString(Op op);

// -----------------------------------------------------------------------
// Expression subclasses

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
// factory helper
Expression CreateMemoryReference(const int& type, Expression loc);
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
    machine.GetMemory().Write(
        IntMemRef(type_, location_->GetIntegerValue(machine)), rvalue);
  }

  int GetIntegerValue(RLMachine& machine) const override {
    return machine.GetMemory().Read(
        IntMemRef(type_, location_->GetIntegerValue(machine)));
  }

  void SetStringValue(RLMachine& machine, const std::string& rval) override {
    machine.GetMemory().Write(
        StrMemoryLocation(type_, location_->GetIntegerValue(machine)), rval);
  }

  std::string GetStringValue(RLMachine& machine) const override {
    return machine.GetMemory().Read(
        StrMemoryLocation(type_, location_->GetIntegerValue(machine)));
  }

  std::string GetDebugString() const override {
    return std::format("{}[{}]", GetBankName(type_),
                       location_->GetDebugString());
  }

  IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const override {
    if (is_string_location(type_))
      throw Error(
          "Request to GetIntegerReferenceIterator() on a string reference!");

    return IntReferenceIterator(&machine.GetMemory(), type_,
                                location_->GetIntegerValue(machine));
  }

  StringReferenceIterator GetStringReferenceIterator(
      RLMachine& machine) const override {
    if (!is_string_location(type_))
      throw Error(
          "Request to GetStringReferenceIterator() on a string reference!");

    return StringReferenceIterator(&machine.GetMemory(), type_,
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
  friend class BinaryExpressionEx;

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
    machine.GetMemory().Write(IntMemRef(type_, location_), rvalue);
  }

  int GetIntegerValue(RLMachine& machine) const override {
    return machine.GetMemory().Read(IntMemRef(type_, location_));
  }

  void SetStringValue(RLMachine& machine, const std::string& rval) override {
    machine.GetMemory().Write(StrMemoryLocation(type_, location_), rval);
  }

  std::string GetStringValue(RLMachine& machine) const override {
    return machine.GetMemory().Read(StrMemoryLocation(type_, location_));
  }

  IntReferenceIterator GetIntegerReferenceIterator(
      RLMachine& machine) const override {
    if (is_string_location(type_))
      throw Error(
          "Request to GetIntegerReferenceIterator() on a string reference!");

    return IntReferenceIterator(&machine.GetMemory(), type_, location_);
  }

  StringReferenceIterator GetStringReferenceIterator(
      RLMachine& machine) const override {
    if (!is_string_location(type_))
      throw Error(
          "Request to GetStringReferenceIterator() on a string reference!");

    return StringReferenceIterator(&machine.GetMemory(), type_, location_);
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    if (is_string_location(type_))
      return "\"" + GetStringValue(machine) + "\"";
    else
      return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return std::format("{}[{}]", GetBankName(type_), location_);
  }

 private:
  int type_;
  int location_;
};

// ----------------------------------------------------------------------
// Binary Expression
// ----------------------------------------------------------------------
// helper
int PerformBinaryOperationOn(char operation, int lhs, int rhs);
class BinaryExpressionEx : public IExpression {
 public:
  // factory
  static Expression Create(const char operation, Expression l, Expression r);

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
    return std::format("{} {} {}", left_->GetDebugString(),
                       ToString(static_cast<Op>(operation_)),
                       right_->GetDebugString());
  }

 private:
  char operation_;
  Expression left_;
  Expression right_;
};

// ----------------------------------------------------------------------
// Unary Expression
// ----------------------------------------------------------------------
class UnaryEx : public IExpression {
 public:
  UnaryEx(const char& op, const Expression ex) : operation_(op), operand_(ex) {}

  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override {
    return PerformUnaryOperationOn(operand_->GetIntegerValue(machine));
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

  int PerformUnaryOperationOn(int int_operand) const {
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

// ----------------------------------------------------------------------
// Simple Assignment
// ----------------------------------------------------------------------
class SimpleAssignEx : public IExpression {
 public:
  SimpleAssignEx(const int& type, const int& loc, const int& val)
      : type_(type), location_(loc), value_(val) {}
  bool is_valid() const override { return true; }

  int GetIntegerValue(RLMachine& machine) const override {
    machine.GetMemory().Write(IntMemRef(type_, location_), value_);
    return value_;
  }

  std::string GetSerializedExpression(RLMachine& machine) const override {
    return IntToBytecode(GetIntegerValue(machine));
  }

  std::string GetDebugString() const override {
    return std::format("{}[{}] = {}", GetBankName(type_), location_, value_);
  }

 private:
  int type_;
  int location_;
  int value_;
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
      if (piece != expression_.back())
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

  virtual std::string GetStringValue(RLMachine& machine) const override {
    if (expression_.size() == 1) {
      try {
        return expression_.front()->GetStringValue(machine);
      } catch (...) {
      }
    }
    throw Error("ComplexEX::GetStringValue() invalid on this object");
  }

  virtual int GetIntegerValue(RLMachine& machine) const override {
    if (expression_.size() == 1) {
      try {
        return expression_.front()->GetIntegerValue(machine);
      } catch (...) {
      }
    }
    throw Error("ComplexEX::GetIntegerValue() invalid on this object");
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

}  // namespace libreallive
