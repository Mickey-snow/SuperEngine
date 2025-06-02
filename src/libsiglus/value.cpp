// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "libsiglus/value.hpp"

#include "utilities/string_utilities.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <functional>
#include <unordered_map>

namespace libsiglus {

std::string List::ToDebugString() const {
  return '[' + Join(",", vals_to_string(vals)) + ']';
}

std::optional<Value> TryEval(OperatorCode op, Value rhs_val) {
  if (!std::holds_alternative<Integer>(rhs_val))
    return std::nullopt;

  int rhs = AsInt(rhs_val);
  switch (op) {
    case OperatorCode::Plus:
      return Value(Integer(+rhs));
    case OperatorCode::Minus:
      return Value(Integer(-rhs));
    case OperatorCode::Inv:
      return Value(Integer(~rhs));
    default:
      return std::nullopt;
  }
}

std::optional<Value> TryEval(Value lhs_val, OperatorCode op, Value rhs_val) {
  // Int op Int
  if (std::holds_alternative<Integer>(lhs_val) &&
      std::holds_alternative<Integer>(rhs_val)) {
    int lhs = AsInt(lhs_val);
    int rhs = AsInt(rhs_val);

    // special case: attempt to divide by 0, result is 0
    if ((op == OperatorCode::Div || op == OperatorCode::Mod) && rhs == 0)
      return Value(Integer(0));

    static const std::unordered_map<OperatorCode, std::function<int(int, int)>>
        opfun{
            {OperatorCode::Plus, std::plus<>()},
            {OperatorCode::Minus, std::minus<>()},
            {OperatorCode::Mult, std::multiplies<>()},
            {OperatorCode::Div, std::divides<>()},
            {OperatorCode::Mod, std::modulus<>()},

            {OperatorCode::And, std::bit_and<>()},
            {OperatorCode::Or, std::bit_or<>()},
            {OperatorCode::Xor, std::bit_xor<>()},
            {OperatorCode::Sr, [](int lhs, int rhs) { return lhs >> rhs; }},
            {OperatorCode::Sl, [](int lhs, int rhs) { return lhs << rhs; }},
            {OperatorCode::Sru,
             [](int lhs, int rhs) -> int {
               return static_cast<unsigned int>(lhs) >> rhs;
             }},

            {OperatorCode::Equal,
             [](int lhs, int rhs) { return lhs == rhs ? 1 : 0; }},
            {OperatorCode::Ne,
             [](int lhs, int rhs) { return lhs != rhs ? 1 : 0; }},
            {OperatorCode::Le,
             [](int lhs, int rhs) { return lhs <= rhs ? 1 : 0; }},
            {OperatorCode::Ge,
             [](int lhs, int rhs) { return lhs >= rhs ? 1 : 0; }},
            {OperatorCode::Lt,
             [](int lhs, int rhs) { return lhs < rhs ? 1 : 0; }},
            {OperatorCode::Gt,
             [](int lhs, int rhs) { return lhs > rhs ? 1 : 0; }},
            {OperatorCode::LogicalAnd,
             [](int lhs, int rhs) { return (lhs && rhs) ? 1 : 0; }},
            {OperatorCode::LogicalOr,
             [](int lhs, int rhs) { return (lhs || rhs) ? 1 : 0; }},
        };
    int result = std::invoke(opfun.at(op), lhs, rhs);
    return Value(Integer(result));

  }

  // String op Int
  else if (std::holds_alternative<String>(lhs_val) &&
           std::holds_alternative<Integer>(rhs_val)) {
    std::string lhs = AsStr(lhs_val);
    int rhs = AsInt(rhs_val);

    if (op == OperatorCode::Mult) {
      std::string result;
      result.reserve(lhs.size() * rhs);
      for (int i = 0; i < rhs; ++i)
        result += lhs;
      return Value(String(std::move(result)));
    }
  }

  // String op String
  else if (std::holds_alternative<String>(lhs_val) &&
           std::holds_alternative<String>(rhs_val)) {
    std::string lhs = AsStr(lhs_val);
    std::string rhs = AsStr(rhs_val);

    if (op == OperatorCode::Plus)
      return Value(String(lhs + rhs));
    else {
      boost::to_lower(lhs);
      boost::to_lower(rhs);

      static const std::unordered_map<OperatorCode,
                                      std::function<int(std::strong_ordering)>>
          opfun{{OperatorCode::Equal,
                 [](std::strong_ordering ord) { return ord == 0 ? 1 : 0; }},
                {OperatorCode::Ne,
                 [](std::strong_ordering ord) { return ord != 0 ? 1 : 0; }},
                {OperatorCode::Le,
                 [](std::strong_ordering ord) { return ord <= 0 ? 1 : 0; }},
                {OperatorCode::Ge,
                 [](std::strong_ordering ord) { return ord >= 0 ? 1 : 0; }},
                {OperatorCode::Lt,
                 [](std::strong_ordering ord) { return ord < 0 ? 1 : 0; }},
                {OperatorCode::Gt,
                 [](std::strong_ordering ord) { return ord > 0 ? 1 : 0; }}};

      int result = std::invoke(opfun.at(op), lhs <=> rhs);
      return Value(Integer(result));
    }
  }

  return std::nullopt;
}

}  // namespace libsiglus
