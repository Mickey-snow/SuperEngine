// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include "libsiglus/assembler.hpp"

#include "libsiglus/lexeme.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <compare>
#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace libsiglus {

Instruction Assembler::Assemble(Lexeme lex) { return std::visit(*this, lex); }

Instruction Assembler::operator()(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(push.value_);
      break;
    case Type::String: {
      const auto str_val = (*str_table_)[push.value_];
      stack_.Push(str_val);
      break;
    }

    default:
      throw std::runtime_error(
          "Assembler: Unknow type id " +
          std::to_string(static_cast<uint32_t>(push.type_)));
  }
  return std::monostate();
}

Instruction Assembler::operator()(lex::Line line) {
  lineno_ = line.linenum_;
  return std::monostate();
}

Instruction Assembler::operator()(lex::Marker marker) {
  stack_.PushMarker();
  return std::monostate();
}

Instruction Assembler::operator()(lex::Command command) {
  Command result;
  result.return_type = command.rettype_;
  result.overload_id = command.override_;

  result.named_arg.resize(command.arg_tag_.size());
  result.arg.resize(command.arg_.size() - command.arg_tag_.size());
  for (auto it = result.named_arg.rbegin(); it != result.named_arg.rend();
       ++it) {
    it->first = command.arg_tag_.back();
    it->second = stack_.Pop(command.arg_.back());
    command.arg_tag_.pop_back();
    command.arg_.pop_back();
  }
  for (auto it = result.arg.rbegin(); it != result.arg.rend(); ++it) {
    *it = stack_.Pop(command.arg_.back());
    command.arg_.pop_back();
  }

  result.elm = stack_.Popelm();

  return result;
}

Instruction Assembler::operator()(lex::Operate1 op) {
  int rhs = stack_.Popint();
  switch (op.op_) {
    case OperatorCode::Plus:
      stack_.Push(+rhs);
      break;
    case OperatorCode::Minus:
      stack_.Push(-rhs);
      break;
    case OperatorCode::Inv:
      stack_.Push(~rhs);
      break;
    default:
      break;
  }
  return std::monostate();
}

Instruction Assembler::operator()(lex::Operate2 op) {
  if (op.ltype_ == Type::Int && op.rtype_ == Type::Int) {
    int rhs = stack_.Popint();
    int lhs = stack_.Popint();
    if ((op.op_ == OperatorCode::Div || op.op_ == OperatorCode::Mod) &&
        rhs == 0) {
      // attempt to divide by 0
      stack_.Push(0);
      return std::monostate();
    }

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
    stack_.Push(std::invoke(opfun.at(op.op_), lhs, rhs));
  } else if (op.ltype_ == Type::String && op.rtype_ == Type::Int) {
    int rhs = stack_.Popint();
    std::string lhs = stack_.Popstr();
    if (op.op_ == OperatorCode::Mult) {
      std::string result;
      result.reserve(lhs.size() * rhs);
      for (int i = 0; i < rhs; ++i)
        result += lhs;
      stack_.Push(std::move(result));
    }
  } else if (op.ltype_ == Type::String && op.rtype_ == Type::String) {
    std::string rhs = stack_.Popstr();
    std::string lhs = stack_.Popstr();

    if (op.op_ == OperatorCode::Plus)
      stack_.Push(lhs + rhs);
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

      stack_.Push(std::invoke(opfun.at(op.op_), lhs <=> rhs));
    }
  }

  return std::monostate();
}

}  // namespace libsiglus
