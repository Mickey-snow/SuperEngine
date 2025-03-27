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

#include "machine/value.hpp"
#include "m6/exception.hpp"
#include "machine/op.hpp"

using namespace m6;

// -----------------------------------------------------------------------
// class IObject

std::string IObject::Str() const { return "<str: ?>"; }
std::string IObject::Desc() const { return "<desc: ?>"; }

// -----------------------------------------------------------------------
// class Value

Value::Value(value_t x) : val_(std::move(x)) {}

std::string Value::Str() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "nil";
        else if constexpr (std::same_as<T, int>)
          return std::to_string(x);
        else if constexpr (std::same_as<T, std::string>)
          return x;
        else if constexpr (std::same_as<T, std::shared_ptr<IObject>>)
          return x->Str();
        else {  // object
          return "???";
        }
      },
      val_);
}

std::string Value::Desc() const {
  return std::visit(
      [](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::monostate>)
          return "<nil>";
        else if constexpr (std::same_as<T, int>)
          return "<int: " + std::to_string(x) + '>';
        else if constexpr (std::same_as<T, std::string>)
          return "<str: " + x + '>';
        else if constexpr (std::same_as<T, std::shared_ptr<IObject>>)
          return x->Desc();
        else
          return "???";
      },
      val_);
}

ObjType Value::Type() const {
  return std::visit(
      [](const auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, int>)
          return ObjType::Int;
        else if constexpr (std::same_as<T, std::string>)
          return ObjType::Str;
        else if constexpr (std::same_as<T, std::shared_ptr<IObject>>)
          return x->Type();
        else
          return ObjType::Nil;
      },
      val_);
}

namespace {
static const Value True(1);
static const Value False(0);

Value handleIntIntOp(Op op, int& lhs, int rhs) {
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

Value handleStringIntOp(Op op, std::string& lhs, int rhs) {
  if ((op == Op::Mul || op == Op::MulAssign) && rhs >= 0) {
    std::string result, current = lhs;
    result.reserve(lhs.size() * rhs);
    int count = rhs;
    while (count > 0) {
      if (count & 1) {
        result += current;
      }
      current += current;
      count >>= 1;
    }
    if (op == Op::MulAssign) {
      lhs = result;
    }
    return Value(std::move(result));
  }

  throw UndefinedOperator(op, {"<str>", std::to_string(rhs)});
}

Value handleStringStringOp(Op op, std::string& lhs, const std::string& rhs) {
  switch (op) {
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::Add: {
      return Value(lhs + rhs);
    }
    case Op::AddAssign: {
      lhs += rhs;
      return Value(lhs);
    }
    default:
      throw UndefinedOperator(op, {lhs, rhs});
  }
}

}  // namespace

Value Value::Operator(Op op, Value rhs) {
  return std::visit(
      [op, &rhs, this](auto& lhsVal) -> Value {
        using LHS = std::decay_t<decltype(lhsVal)>;

        if constexpr (false)
          ;

        if constexpr (std::same_as<LHS, int>) {
          if (auto rhsIntPtr = std::get_if<int>(&rhs.val_)) {
            return handleIntIntOp(op, lhsVal, *rhsIntPtr);
          }
        }

        else if constexpr (std::same_as<LHS, std::string>) {
          if (auto rhsIntPtr = std::get_if<int>(&rhs.val_)) {
            return handleStringIntOp(op, lhsVal, *rhsIntPtr);
          } else if (auto rhsStrPtr = std::get_if<std::string>(&rhs.val_)) {
            return handleStringStringOp(op, lhsVal, *rhsStrPtr);
          }
        }

        throw UndefinedOperator(op, {this->Desc(), rhs.Desc()});
      },
      val_);
}

Value Value::Operator(Op op) {
  return std::visit(
      [op, this](const auto& x) -> Value {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, int>) {
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

        throw UndefinedOperator(op, {this->Desc()});
      },
      val_);
}

Value::operator std::string() const { return this->Desc(); }

bool Value::operator==(int rhs) const {
  auto ptr = std::get_if<int>(&val_);
  return ptr && *ptr == rhs;
}

bool Value::operator==(const std::string& rhs) const {
  auto ptr = std::get_if<std::string>(&val_);
  return ptr && *ptr == rhs;
}
