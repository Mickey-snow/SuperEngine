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

#include "libsiglus/parser.hpp"

#include "libsiglus/lexeme.hpp"
#include "log/domain_logger.hpp"
#include "utilities/string_utilities.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <compare>
#include <format>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>

namespace libsiglus {
namespace ast {
std::string Command::ToDebugString() const {
  const std::string cmd_repr = std::format(
      "cmd<{}:{}>", Join(",", elm | std::views::transform([](const auto& x) {
                                return std::to_string(x);
                              })),
      overload_id);

  std::vector<std::string> args_repr;
  args_repr.reserve(arg.size() + named_arg.size());

  std::transform(
      arg.cbegin(), arg.cend(), std::back_inserter(args_repr),
      [](const Value& value) { return std::visit(DebugStringOf(), value); });
  std::transform(named_arg.cbegin(), named_arg.cend(),
                 std::back_inserter(args_repr), [](const auto& it) {
                   return std::format("_{}={}", it.first,
                                      std::visit(DebugStringOf(), it.second));
                 });

  return std::format("{}({}) -> {}", cmd_repr, Join(",", args_repr),
                     ToString(return_type));
}
std::string Textout::ToDebugString() const {
  return std::format("Textout@{} ({})", kidoku, str);
}
std::string GetProperty::ToDebugString() const {
  return std::format("get({})", Join(",", std::views::all(elm) |
                                              std::views::transform([](int x) {
                                                return std::to_string(x);
                                              })));
}
std::string Goto::ToDebugString() const {
  return "goto .L" + std::to_string(label);
}
std::string Label::ToDebugString() const { return ".L" + std::to_string(id); }
}  // namespace ast
using namespace ast;

// -----------------------------------------------------------------------
// class Parser
static DomainLogger logger("Parser");

Parser::Parser(Scene& scene) : scene_(scene), reader_(scene_.scene_) {}

void Parser::ParseAll() {
  while (reader_.Position() < reader_.Size())
    std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
               lexer_.Parse(reader_));
}

void Parser::Add(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(push.value_);
      break;
    case Type::String: {
      const auto str_val = scene_.str_[push.value_];
      stack_.Push(str_val);
      break;
    }

    default:  // ignore
      break;
  }
}

void Parser::Add(lex::Pop pop) {
  switch (pop.type_) {
    case Type::Int:
      stack_.Popint();
      break;
    case Type::String:
      stack_.Popstr();
      break;
    default:  // ignore
      break;
  }
}

void Parser::Add(lex::Line line) { lineno_ = line.linenum_; }

void Parser::Add(lex::Marker marker) { stack_.PushMarker(); }

void Parser::Add(lex::Property) {
  auto elm = stack_.Popelm();

  const int user_prop_flag = (elm.front() >> 24) & 0xFF;
  if (user_prop_flag != 0x7F) {
    logger(Severity::Error)
        << "Expected user property to have flag 0x7F, but got " << std::hex
        << user_prop_flag;
  }
  elm.front() &= 0x00FFFFFF;

  token_.emplace_back(GetProperty{std::move(elm)});
}

void Parser::Add(lex::Command command) {
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

  auto peek = lexer_.Parse(reader_);
  if (auto p = std::get_if<lex::Pop>(&peek); p->type_ == result.return_type) {
    // return value ignored
  } else {
    // not implemented yet
    throw std::runtime_error("Parser: ret val needed for command " +
                             result.ToDebugString());
  }

  token_append(std::move(result));
}

void Parser::Add(lex::Operate1 op) {
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
}

void Parser::Add(lex::Operate2 op) {
  if (op.ltype_ == Type::Int && op.rtype_ == Type::Int) {
    int rhs = stack_.Popint();
    int lhs = stack_.Popint();
    if ((op.op_ == OperatorCode::Div || op.op_ == OperatorCode::Mod) &&
        rhs == 0) {
      // attempt to divide by 0, result should be 0
      stack_.Push(0);
      return;
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
}

void Parser::Add(lex::Copy cp) {
  switch (cp.type_) {
    case Type::Int:
      stack_.Push(stack_.Backint());
      break;
    case Type::String:
      stack_.Push(stack_.Backstr());
      break;

    default:
      break;
  }
}

void Parser::Add(lex::CopyElm) { stack_.Push(stack_.Backelm()); }

void Parser::Add(lex::Namae) { token_append(Name{stack_.Popstr()}); }

void Parser::Add(lex::Textout t) {
  token_append(Textout{.kidoku = t.kidoku_, .str = stack_.Popstr()});
}

std::string Parser::DumpTokens() const {
  std::string result;
  for (const auto& it : token_)
    result +=
        std::visit([](const auto& v) { return ToDebugString(v); }, it) + "\n";
  return result;
}

}  // namespace libsiglus
