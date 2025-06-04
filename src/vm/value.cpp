// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "vm/value.hpp"

// TODO: Reconsider exception thrown here
#include "m6/exception.hpp"
#include "vm/iobject.hpp"
#include "vm/object.hpp"

#include <cmath>

namespace serilang {

using namespace m6;

// -----------------------------------------------------------------------
// class Value

Value::Value(value_t x) : val_(std::move(x)) {}

std::string Value::Str() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "nil";
        else if constexpr (std::same_as<T, bool>)
          return x ? "true" : "false";
        else if constexpr (std::same_as<T, int>)
          return std::to_string(x);
        else if constexpr (std::same_as<T, double>)
          return std::to_string(x);
        else if constexpr (std::same_as<T, std::string>)
          return x;
        else if constexpr (std::same_as<T, IObject*>)
          return x->Str();
      },
      val_);
}

std::string Value::Desc() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "<nil>";
        else if constexpr (std::same_as<T, bool>)
          return x ? "<bool: true>" : "<bool: false>";
        else if constexpr (std::same_as<T, int>)
          return "<int: " + std::to_string(x) + '>';
        else if constexpr (std::same_as<T, double>)
          return "<double: " + std::to_string(x) + '>';
        else if constexpr (std::same_as<T, std::string>)
          return "<str: " + x + '>';
        else if constexpr (std::same_as<T, IObject*>)
          return x->Desc();
      },
      val_);
}

bool Value::IsTruthy() const {
  return std::visit(
      [](const auto& x) -> bool {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return false;
        else if constexpr (std::same_as<T, bool>)
          return x;
        else if constexpr (std::same_as<T, int>)
          return x != 0;
        else if constexpr (std::same_as<T, double>)
          return x != 0.0;
        else if constexpr (std::same_as<T, std::string>)
          return !x.empty();
        else if constexpr (std::same_as<T, IObject*>)
          return x != nullptr;
      },
      val_);
}

ObjType Value::Type() const {
  return std::visit(
      [](const auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, bool>)
          return ObjType::Bool;
        else if constexpr (std::same_as<T, int>)
          return ObjType::Int;
        else if constexpr (std::same_as<T, double>)
          return ObjType::Double;
        else if constexpr (std::same_as<T, std::string>)
          return ObjType::Str;
        else if constexpr (std::same_as<T, IObject*>)
          return x->Type();
        else
          return ObjType::Nil;
      },
      val_);
}

namespace {
static const Value True(true);
static const Value False(false);

Value handleIntIntOp(Op op, int lhs, int rhs) {
  switch (op) {
    // Arithmetic
    case Op::Add:
      return Value(lhs + rhs);
    case Op::Sub:
      return Value(lhs - rhs);
    case Op::Mul:
      return Value(lhs * rhs);
    case Op::Div:
      return (rhs == 0) ? Value(0) : Value(lhs / rhs);
    case Op::Mod:
      return Value(lhs % rhs);
    case Op::Pow: {
      int result = 1;
      while (rhs) {
        if (rhs & 1)
          result *= lhs;
        lhs *= lhs;
        rhs >>= 1;
      }
      return Value(result);
    }

    // Bitwise
    case Op::BitAnd:
      return Value(lhs & rhs);
    case Op::BitOr:
      return Value(lhs | rhs);
    case Op::BitXor:
      return Value(lhs ^ rhs);
    case Op::ShiftLeft: {
      if (rhs < 0) {
        throw ValueError("negative shift count: " + std::to_string(rhs));
      }
      int shifted = lhs << rhs;
      return Value(shifted);
    }
    case Op::ShiftRight: {
      if (rhs < 0) {
        throw ValueError("negative shift count: " + std::to_string(rhs));
      }
      int shifted = lhs >> rhs;
      return Value(shifted);
    }
    case Op::ShiftUnsignedRight: {
      if (rhs < 0) {
        throw ValueError("negative shift count: " + std::to_string(rhs));
      }
      uint32_t uval = std::bit_cast<uint32_t>(lhs);
      uint32_t shifted = uval >> rhs;
      return Value(static_cast<int>(shifted));
    }

    // Comparisons
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::LessEqual:
      return (lhs <= rhs) ? True : False;
    case Op::Less:
      return (lhs < rhs) ? True : False;
    case Op::GreaterEqual:
      return (lhs >= rhs) ? True : False;
    case Op::Greater:
      return (lhs > rhs) ? True : False;

    // Logical
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;

    default:
      throw UndefinedOperator(op, {std::to_string(lhs), std::to_string(rhs)});
  }
}

Value handleStringIntOp(Op op, std::string lhs, int rhs) {
  if (op == Op::Mul && rhs >= 0) {
    std::string result;
    result.reserve(lhs.size() * rhs);
    lhs.reserve(lhs.size() * rhs);
    while (rhs > 0) {
      if (rhs & 1) {
        result += lhs;
      }
      lhs += lhs;
      rhs >>= 1;
    }
    return Value(std::move(result));
  }

  throw UndefinedOperator(op, {"<str>", std::to_string(rhs)});
}

Value handleStringStringOp(Op op,
                           const std::string& lhs,
                           const std::string& rhs) {
  switch (op) {
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::Add:
      return Value(lhs + rhs);
    default:
      throw UndefinedOperator(op, {lhs, rhs});
  }
}

Value handleDoubleDoubleOp(Op op, double lhs, double rhs) {
  switch (op) {
    // Arithmetic
    case Op::Add:
      return Value(lhs + rhs);
    case Op::Sub:
      return Value(lhs - rhs);
    case Op::Mul:
      return Value(lhs * rhs);
    case Op::Div:
      return (rhs == 0.0) ? Value(0.0) : Value(lhs / rhs);
    case Op::Mod:
      return Value(std::fmod(lhs, rhs));

    // Comparisons
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::LessEqual:
      return (lhs <= rhs) ? True : False;
    case Op::Less:
      return (lhs < rhs) ? True : False;
    case Op::GreaterEqual:
      return (lhs >= rhs) ? True : False;
    case Op::Greater:
      return (lhs > rhs) ? True : False;

    // Logical
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;

    default:
      throw UndefinedOperator(op, {std::to_string(lhs), std::to_string(rhs)});
  }
}

Value handleBoolBoolOp(Op op, bool lhs, bool rhs) {
  switch (op) {
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    default:
      throw UndefinedOperator(op,
                              {lhs ? "true" : "false", rhs ? "true" : "false"});
  }
}

}  // namespace

TempValue Value::Operator(Op op, Value rhs) {
  Value result = std::visit(
      [op, &rhs, this](auto& lhsVal) -> Value {
        using LHS = std::decay_t<decltype(lhsVal)>;

        if constexpr (false)
          ;

        // ---------- int ----------
        else if constexpr (std::same_as<LHS, int>) {
          if (auto rhsInt = std::get_if<int>(&rhs.val_))
            return handleIntIntOp(op, lhsVal, *rhsInt);
          if (auto rhsDouble = std::get_if<double>(&rhs.val_)) {
            double lhsd = static_cast<double>(lhsVal);
            return handleDoubleDoubleOp(op, lhsd, *rhsDouble);
          }
          if (auto rhsBool = std::get_if<bool>(&rhs.val_)) {
            int rhsi = *rhsBool ? 1 : 0;
            return handleIntIntOp(op, lhsVal, rhsi);
          }
        }

        // -------- double ---------
        else if constexpr (std::same_as<LHS, double>) {
          if (auto rhsDouble = std::get_if<double>(&rhs.val_))
            return handleDoubleDoubleOp(op, lhsVal, *rhsDouble);
          if (auto rhsInt = std::get_if<int>(&rhs.val_))
            return handleDoubleDoubleOp(op, lhsVal,
                                        static_cast<double>(*rhsInt));
          if (auto rhsBool = std::get_if<bool>(&rhs.val_)) {
            double rhsd = rhsBool ? 1.0 : 0.0;
            return handleDoubleDoubleOp(op, lhsVal, rhsd);
          }
        }

        // ---------- bool ----------
        else if constexpr (std::same_as<LHS, bool>) {
          if (auto rhsBool = std::get_if<bool>(&rhs.val_))
            return handleBoolBoolOp(op, lhsVal, *rhsBool);
          if (auto rhsInt = std::get_if<int>(&rhs.val_))
            return handleIntIntOp(op, (lhsVal ? 1 : 0), *rhsInt);
          if (auto rhsDouble = std::get_if<double>(&rhs.val_))
            return handleDoubleDoubleOp(op, (lhsVal ? 1.0 : 0.0), *rhsDouble);
        }

        // -------- string ---------
        else if constexpr (std::same_as<LHS, std::string>) {
          if (auto rhsInt = std::get_if<int>(&rhs.val_))
            return handleStringIntOp(op, lhsVal, *rhsInt);
          if (auto rhsStr = std::get_if<std::string>(&rhs.val_))
            return handleStringStringOp(op, lhsVal, *rhsStr);
        }

        throw UndefinedOperator(op, {this->Desc(), rhs.Desc()});
      },
      val_);

  return TempValue(std::move(result));
}

TempValue Value::Operator(Op op) {
  Value result = std::visit(
      [op, this](const auto& x) -> Value {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        // ---------- int ----------
        else if constexpr (std::same_as<T, int>) {
          switch (op) {
            case Op::Add:
              return Value(x);
            case Op::Sub:
              return Value(-x);
            case Op::Tilde:
              return Value(~x);
            default:
              break;
          }
        }

        // -------- double ---------
        else if constexpr (std::same_as<T, double>) {
          switch (op) {
            case Op::Add:
              return Value(x);
            case Op::Sub:
              return Value(-x);
            default:
              break;
          }
        }

        // ---------- bool ----------
        else if constexpr (std::same_as<T, bool>) {
          switch (op) {
            case Op::Tilde:  // use ~ as logical NOT
              return Value(!x);
            default:
              break;
          }
        }

        throw UndefinedOperator(op, {this->Desc()});
      },
      val_);

  return TempValue(std::move(result));
}

void Value::Call(VM& vm, Fiber& f, uint8_t nargs, uint8_t nkwargs) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->Call(vm, f, nargs, nkwargs);

        else
          throw std::runtime_error('\'' + Desc() + "' object is not callable.");
      },
      val_);
}

TempValue Value::Item(const Value& idx) {
  return std::visit(
      [&](auto& x) -> TempValue {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          return x->Item(idx);

        throw std::runtime_error('\'' + Desc() +
                                 "' object does not support item assignment.");
      },
      val_);
}

TempValue Value::Member(std::string_view mem) {
  return std::visit(
      [&](auto& x) -> TempValue {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          return x->Member(mem);

        else
          throw std::runtime_error('\'' + Desc() + "' object has no member '" +
                                   std::string(mem) + '\'');
      },
      val_);
}

void Value::SetMember(std::string_view mem, Value value) {
  std::visit(
      [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, IObject*>)
          x->SetMember(mem, std::move(value));

        else
          throw std::runtime_error(
              '\'' + Desc() + "' object does not support member assignment.");
      },
      val_);
}

Value::operator std::string() const { return this->Desc(); }

bool Value::operator==(std::monostate) const {
  return std::holds_alternative<std::monostate>(val_);
}

bool Value::operator==(int rhs) const {
  auto ptr = std::get_if<int>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(double rhs) const {
  auto ptr = std::get_if<double>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(bool rhs) const {
  auto ptr = std::get_if<bool>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(const std::string& rhs) const {
  auto ptr = std::get_if<std::string>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(char const* s) const {
  auto ptr = std::get_if<std::string>(&val_);
  return ptr && *ptr == s;
}

}  // namespace serilang
