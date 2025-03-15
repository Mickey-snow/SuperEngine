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

// factory methods
Value_ptr make_value(int value) { return std::make_shared<Value>(value); }
Value_ptr make_value(std::string value) {
  return std::make_shared<Value>(std::move(value));
}

Value_ptr make_value(char const* value) {
  return make_value(std::string{value});
}
Value_ptr make_value(bool value) { return make_value(value ? 1 : 0); }

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

std::any Value::Get() const {
  return std::visit([](const auto& x) -> std::any { return x; }, val_);
}

void* Value::Getptr() {
  return std::visit([](auto& x) -> void* { return &x; }, val_);
}

Value Value::Operator(Op op, Value rhs) {
  if (op == Op::Comma)
    return rhs;

  return std::visit(
      [op, this, &rhs](auto& x) -> Value {
        using T = std::decay_t<decltype(x)>;
        static const auto True = Value(1);
        static const auto False = Value(0);

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, int>) {
          if (!std::holds_alternative<int>(rhs.val_))
            goto end;

          const int rhs_val = std::get<int>(rhs.val_);
          switch (op) {
            case Op::Comma:
              return rhs;

            case Op::Add:
              return Value(x + rhs_val);
            case Op::AddAssign:
              return Value(x += rhs_val);
            case Op::Sub:
              return Value(x - rhs_val);
            case Op::SubAssign:
              return Value(x -= rhs_val);
            case Op::Mul:
              return Value(x * rhs_val);
            case Op::MulAssign:
              return Value(x *= rhs_val);
            case Op::Div: {
              if (rhs_val == 0)
                return Value(0);
              return Value(x / rhs_val);
            }
            case Op::DivAssign: {
              if (rhs_val == 0)
                return Value(x = 0);
              return Value(x /= rhs_val);
            }
            case Op::Mod:
              return Value(x % rhs_val);
            case Op::ModAssign:
              return Value(x %= rhs_val);
            case Op::BitAnd:
              return Value(x & rhs_val);
            case Op::BitAndAssign:
              return Value(x &= rhs_val);
            case Op::BitOr:
              return Value(x | rhs_val);
            case Op::BitOrAssign:
              return Value(x |= rhs_val);
            case Op::BitXor:
              return Value(x ^ rhs_val);
            case Op::BitXorAssign:
              return Value(x ^= rhs_val);
            case Op::ShiftLeft:
            case Op::ShiftLeftAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const auto result = x << rhs_val;
              if (op == Op::ShiftLeftAssign)
                x = result;
              return Value(result);
            }
            case Op::ShiftRight:
            case Op::ShiftRightAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const auto result = x >> rhs_val;
              if (op == Op::ShiftRightAssign)
                x = result;
              return Value(result);
            }
            case Op::ShiftUnsignedRight:
            case Op::ShiftUnsignedRightAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const int result = std::bit_cast<uint32_t>(x) >> rhs_val;
              if (op == Op::ShiftUnsignedRightAssign)
                x = result;
              return Value(result);
            }
            case Op::Equal:
              return x == rhs_val ? True : False;
            case Op::NotEqual:
              return x != rhs_val ? True : False;
            case Op::LessEqual:
              return x <= rhs_val ? True : False;
            case Op::Less:
              return x < rhs_val ? True : False;
            case Op::GreaterEqual:
              return x >= rhs_val ? True : False;
            case Op::Greater:
              return x > rhs_val ? True : False;

            case Op::LogicalAnd:
              return x && rhs_val ? True : False;
            case Op::LogicalOr:
              return x || rhs_val ? True : False;

            default:
              break;
          }
        }

        else if constexpr (std::same_as<T, std::string>) {
          if (std::holds_alternative<int>(rhs.val_)) {
            auto rhs_value = std::any_cast<int>(rhs.Get());
            if (rhs_value >= 0 && (op == Op::Mul || op == Op::MulAssign)) {
              std::string result, current = x;
              result.reserve(x.size() * rhs_value);
              while (rhs_value) {
                if (rhs_value & 1)
                  result += current;
                current = current + current;
                rhs_value >>= 1;
              }

              if (op == Op::MulAssign)
                x = result;
              return Value(std::move(result));
            }

          }

          else if (std::holds_alternative<std::string>(rhs.val_)) {
            const auto rhs_value = std::any_cast<std::string>(rhs.Get());

            switch (op) {
              case Op::Equal:
                return x == rhs_value ? True : False;
              case Op::NotEqual:
                return x != rhs_value ? True : False;
              case Op::Add:
                return Value(x + rhs_value);
              case Op::AddAssign:
                x += rhs_value;
                return Value(x);
              default:
                break;
            }
          }
        }

      [[maybe_unused]] end:
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
