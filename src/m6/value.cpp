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
#include "m6/value_error.hpp"

namespace m6 {
// factory methods
Value make_value(int value) { return Value(std::make_any<int>(value)); }
Value make_value(std::string value) {
  return Value(std::make_any<std::string>(std::move(value)));
}

// -----------------------------------------------------------------------
// class Value
Value::Value(std::any val) : val_(std::move(val)) {}

std::type_index Value::Type() const noexcept { return val_.type(); }

std::string Value::Str() const {
  if (Type() == std::type_index(typeid(int)))
    return std::to_string(std::any_cast<int>(val_));
  else if (Type() == std::type_index(typeid(std::string)))
    return std::any_cast<std::string>(val_);
  else
    return "???";
}

std::string Value::Desc() const {
  if (Type() == std::type_index(typeid(int)))
    return "Int";
  else if (Type() == std::type_index(typeid(std::string)))
    return "Str";
  else
    return "???";
}

bool Value::operator==(int rhs) const noexcept {
  try {
    return std::any_cast<int>(val_) == rhs;
  } catch (std::exception& e) {
    return false;
  }
}

bool Value::operator==(std::string_view rhs) const noexcept {
  try {
    return std::any_cast<std::string>(val_) == rhs;
  } catch (std::exception& e) {
    return false;
  }
}

// -----------------------------------------------------------------------
// Calculate
Value Value::Calculate(Op op, const Value& self) {
  try {
    const auto rhs_val = std::any_cast<int>(self.val_);
    switch (op) {
      case Op::Add:
        return make_value(rhs_val);
      case Op::Sub:
        return make_value(-rhs_val);
      case Op::Tilde:
        return make_value(~rhs_val);

      default:
        break;
    }
  } catch (std::bad_any_cast& e) {
  }

  throw UndefinedOperator(op, {self.Desc()});
}
Value Value::Calculate(const Value& lhs, Op op, const Value& rhs) {
  if (op == Op::Comma)
    return rhs;

  const static Value True = make_value(1);
  const static Value False = make_value(0);

  try {
    if (lhs.Type() == typeid(std::string) ||
        rhs.Type() == typeid(std::string)) {
      switch (op) {
        case Op::Equal: {
          const auto result = std::any_cast<std::string>(lhs.val_) ==
                              std::any_cast<std::string>(rhs.val_);
          return make_value(result ? 1 : 0);
        }
        case Op::NotEqual: {
          const auto result = std::any_cast<std::string>(lhs.val_) !=
                              std::any_cast<std::string>(rhs.val_);
          return make_value(result ? 1 : 0);
        }

        case Op::Add:
          return make_value(std::any_cast<std::string>(lhs.val_) +
                            std::any_cast<std::string>(rhs.val_));

        case Op::Mul: {
          auto section = std::any_cast<std::string>(lhs.val_);
          auto reps = std::any_cast<int>(rhs.val_);
          if (reps < 0)
            break;

          std::string result;
          result.reserve(reps * section.size());
          while (reps) {
            if (reps & 1)
              result += section;
            section = section + section;
            reps >>= 1;
          }

          return make_value(std::move(result));
        }
        default:
          break;
      }
    }

    const auto lhs_val = std::any_cast<int>(lhs.val_);
    const auto rhs_val = std::any_cast<int>(rhs.val_);
    switch (op) {
      case Op::Comma:
        return rhs;

      case Op::Add:
        return make_value(lhs_val + rhs_val);
      case Op::Sub:
        return make_value(lhs_val - rhs_val);
      case Op::Mul:
        return make_value(lhs_val * rhs_val);
      case Op::Div: {
        if (rhs_val == 0)
          return make_value(0);
        return make_value(lhs_val / rhs_val);
      }
      case Op::Mod:
        return make_value(lhs_val % rhs_val);
      case Op::BitAnd:
        return make_value(lhs_val & rhs_val);
      case Op::BitOr:
        return make_value(lhs_val | rhs_val);
      case Op::BitXor:
        return make_value(lhs_val ^ rhs_val);
      case Op::ShiftLeft:
        return make_value(lhs_val << rhs_val);
      case Op::ShiftRight:
        return make_value(lhs_val >> rhs_val);

      case Op::Equal:
        return lhs_val == rhs_val ? True : False;
      case Op::NotEqual:
        return lhs_val != rhs_val ? True : False;
      case Op::LessEqual:
        return lhs_val <= rhs_val ? True : False;
      case Op::Less:
        return lhs_val < rhs_val ? True : False;
      case Op::GreaterEqual:
        return lhs_val >= rhs_val ? True : False;
      case Op::Greater:
        return lhs_val > rhs_val ? True : False;

      case Op::LogicalAnd:
        return lhs_val && rhs_val ? True : False;
      case Op::LogicalOr:
        return lhs_val || rhs_val ? True : False;

      default:
        break;
    }
  } catch (std::bad_any_cast& e) {
  }

  throw UndefinedOperator(op, {lhs.Desc(), rhs.Desc()});
}

}  // namespace m6
