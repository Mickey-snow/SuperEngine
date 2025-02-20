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

#include "m6/value.hpp"
#include "m6/exception.hpp"
#include "m6/op.hpp"

namespace m6 {
// factory methods
Value_ptr make_value(int value) { return std::make_shared<Value>(value); }
Value_ptr make_value(std::string value) {
  return std::make_shared<Value>(std::move(value));
}

Value_ptr make_value(char const* value) { return make_value(std::string{value}); }
Value_ptr make_value(bool value) { return make_value(value ? 1 : 0); }

// -----------------------------------------------------------------------
// class IValue

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
      },
      val_);
}

std::type_index Value::Type() const {
  return std::visit(
      [](const auto& x) -> std::type_index {
        using T = std::decay_t<decltype(x)>;
        return typeid(T);
      },
      val_);
}

Value_ptr Value::Duplicate() { return std::make_shared<Value>(val_); }

std::any Value::Get() const {
  return std::visit([](const auto& x) -> std::any { return x; }, val_);
}

void* Value::Getptr() {
  return std::visit([](auto& x) -> void* { return &x; }, val_);
}

Value_ptr Value::Operator(Op op, Value_ptr rhs) {
  if (op == Op::Comma)
    return rhs;

  return std::visit(
      [op, this, &rhs](auto& x) -> Value_ptr {
        using T = std::decay_t<decltype(x)>;

        if constexpr (false)
          ;

        else if constexpr (std::same_as<T, int>) {
          if (!std::holds_alternative<int>(rhs->val_))
            goto end;

          const int rhs_val = std::get<int>(rhs->val_);
          static const auto True = make_value(true);
          static const auto False = make_value(false);

          switch (op) {
            case Op::Comma:
              return rhs;

            case Op::Add:
              return make_value(x + rhs_val);
            case Op::AddAssign:
              return make_value(x += rhs_val);
            case Op::Sub:
              return make_value(x - rhs_val);
            case Op::SubAssign:
              return make_value(x -= rhs_val);
            case Op::Mul:
              return make_value(x * rhs_val);
            case Op::MulAssign:
              return make_value(x *= rhs_val);
            case Op::Div: {
              if (rhs_val == 0)
                return make_value(0);
              return make_value(x / rhs_val);
            }
            case Op::DivAssign: {
              if (rhs_val == 0)
                return make_value(x = 0);
              return make_value(x /= rhs_val);
            }
            case Op::Mod:
              return make_value(x % rhs_val);
            case Op::ModAssign:
              return make_value(x %= rhs_val);
            case Op::BitAnd:
              return make_value(x & rhs_val);
            case Op::BitAndAssign:
              return make_value(x &= rhs_val);
            case Op::BitOr:
              return make_value(x | rhs_val);
            case Op::BitOrAssign:
              return make_value(x |= rhs_val);
            case Op::BitXor:
              return make_value(x ^ rhs_val);
            case Op::BitXorAssign:
              return make_value(x ^= rhs_val);
            case Op::ShiftLeft:
            case Op::ShiftLeftAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const auto result = x << rhs_val;
              if (op == Op::ShiftLeftAssign)
                x = result;
              return make_value(result);
            }
            case Op::ShiftRight:
            case Op::ShiftRightAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const auto result = x >> rhs_val;
              if (op == Op::ShiftRightAssign)
                x = result;
              return make_value(result);
            }
            case Op::ShiftUnsignedRight:
            case Op::ShiftUnsignedRightAssign: {
              if (rhs_val < 0)
                throw ValueError("negative shift count: " +
                                 std::to_string(rhs_val));
              const int result = std::bit_cast<uint32_t>(x) >> rhs_val;
              if (op == Op::ShiftUnsignedRightAssign)
                x = result;
              return make_value(result);
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
          if (std::holds_alternative<int>(rhs->val_)) {
            auto rhs_value = std::any_cast<int>(rhs->Get());
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
              return make_value(std::move(result));
            }

          }

          else if (std::holds_alternative<std::string>(rhs->val_)) {
            const auto rhs_value = std::any_cast<std::string>(rhs->Get());
            static const auto True = make_value(1);
            static const auto False = make_value(0);

            switch (op) {
              case Op::Equal:
                return x == rhs_value ? True : False;
              case Op::NotEqual:
                return x != rhs_value ? True : False;
              case Op::Add:
                return make_value(x + rhs_value);
              case Op::AddAssign:
                x += rhs_value;
                return make_value(x);
              default:
                break;
            }
          }
        }

      [[maybe_unused]] end:
        throw UndefinedOperator(op, {this->Desc(), rhs->Desc()});
      },
      val_);
}

Value_ptr Value::Operator(Op op) {
  return std::visit(
      [op, this](const auto& x) -> Value_ptr {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, int>) {
          switch (op) {
            case Op::Add:
              return make_value(x);
            case Op::Sub:
              return make_value(-x);
            case Op::Tilde:
              return make_value(~x);
            default:
              break;
          }
        }

        throw UndefinedOperator(op, {this->Desc()});
      },
      val_);
}

Value_ptr Value::Invoke(std::vector<Value_ptr> args) {
  throw TypeError(Desc() + " object is not callable.");
}

}  // namespace m6
