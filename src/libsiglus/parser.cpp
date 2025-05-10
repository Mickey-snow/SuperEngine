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

#include "libsiglus/archive.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/scene.hpp"
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
namespace token {
std::string Command::ToDebugString() const {
  const std::string cmd_repr = std::format(
      "cmd<{}:{}>", Join(",", elm | std::views::transform([](const auto& x) {
                                return std::to_string(x);
                              })),
      overload_id);

  std::vector<std::string> args_repr;
  args_repr.reserve(arg.size() + named_arg.size());

  std::transform(arg.cbegin(), arg.cend(), std::back_inserter(args_repr),
                 [](const Value& value) { return ToString(value); });
  std::transform(named_arg.cbegin(), named_arg.cend(),
                 std::back_inserter(args_repr), [](const auto& it) {
                   return std::format("_{}={}", it.first, ToString(it.second));
                 });

  return std::format("{} {} = {}({})", ToString(return_type), ToString(dst),
                     cmd_repr, Join(",", args_repr));
}
std::string Textout::ToDebugString() const {
  return std::format("Textout@{} ({})", kidoku, str);
}
std::string GetProperty::ToDebugString() const {
  return std::format("{} {} = {}", ToString(prop.form), ToString(dst),
                     prop.name);
}
std::string Goto::ToDebugString() const {
  return "goto .L" + std::to_string(label);
}
std::string GotoIf::ToDebugString() const {
  return std::format("{}({}) goto .L{}", cond ? "if" : "ifnot", ToString(src),
                     label);
}
std::string Label::ToDebugString() const { return ".L" + std::to_string(id); }
std::string Operate1::ToDebugString() const {
  return std::format("{} {} = {} {}", ToString(Typeof(dst)), ToString(dst),
                     ToString(op), ToString(rhs));
}
std::string Operate2::ToDebugString() const {
  return std::format("{} {} = {} {} {}", ToString(Typeof(dst)), ToString(dst),
                     ToString(lhs), ToString(op), ToString(rhs));
}
std::string Assign::ToDebugString() const {
  return std::format("<{}> = {}", Join(",", view_to_string(dst)),
                     ToString(src));
}
}  // namespace token
using namespace token;

// -----------------------------------------------------------------------
// class Parser
static DomainLogger logger("Parser");

Parser::Parser(Archive& archive, Scene& scene)
    : archive_(archive), scene_(scene), reader_(scene_.scene_) {
  for (size_t i = 0; i < scene.label.size(); ++i) {
    // offset start at 1?
    label_at_.emplace(scene.label[i], i + 1);  // (location, lid)
  }
}

Value Parser::add_var(Type type) {
  Variable var(type, var_cnt_++);
  return var;
}

Value Parser::pop(Type type) {
  switch (type) {
    case Type::Int:
      return stack_.Popint();
    case Type::String:
      return stack_.Popstr();
    default:
      throw std::runtime_error("Parser: unknown pop type " + ToString(type));
  }
}

void Parser::add_label(int id) { token_append(Label{id}); }

void Parser::ParseAll() {
  while (reader_.Position() < reader_.Size()) {
    // Add labels
    for (auto [begin, end] = label_at_.equal_range(reader_.Position());
         begin != end; ++begin) {
      add_label(begin->second);
    }

    // parse
    std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
               lexer_.Parse(reader_));
  }
}

void Parser::Add(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(Integer(push.value_));
      break;
    case Type::String: {
      const auto str_val = scene_.str_[push.value_];
      stack_.Push(String(str_val));
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

void Parser::Add(lex::Line line) {
  lineno_ = line.linenum_;
  // is it safe to assume the stack is empty here?
  if (!stack_.Empty()) {
    logger(Severity::Info) << "at line " << lineno_ << '#' << token_.size()
                           << ", expected stack to be empty.\n"
                           << stack_.ToDebugString();
    stack_.Clear();
  }
}

void Parser::Add(lex::Marker marker) { stack_.PushMarker(); }

void Parser::Add(lex::Property) {
  auto elm = stack_.Popelm();

  const int user_prop_flag = (elm.front() >> 24) & 0xFF;
  if (user_prop_flag != 0x7F) {
    logger(Severity::Error)
        << "at " << token_.size() << ':'
        << "Expected user property to have flag 0x7F, but got " << std::hex
        << user_prop_flag;
  }
  elm.front() &= 0x00FFFFFF;

  Property* prop = nullptr;
  int idx = elm.front();
  if (idx < archive_.prop_.size()) {
    // global "incprop"
    prop = archive_.prop_.data() + idx;
  } else {
    // scene property
    idx -= archive_.prop_.size();
    prop = scene_.property.data() + idx;
  }

  Value dst = add_var(prop->form);
  token_.emplace_back(GetProperty{*prop, dst});

  stack_.Push(dst);
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
  result.dst = add_var(result.return_type);
  stack_.Push(result.dst);

  token_append(std::move(result));
}

void Parser::Add(lex::Operate1 op) {  // + - ~ <int>
  token::Operate1 stmt;
  stmt.rhs = stack_.Popint();
  stmt.dst = add_var(Type::Int);
  stmt.op = op.op_;

  stack_.Push(stmt.dst);
  token_append(std::move(stmt));
}

void Parser::Add(lex::Operate2 op) {
  Type result_type = Type::None;

  if (op.ltype_ == Type::Int && op.rtype_ == Type::Int)
    result_type = Type::Int;
  else if (op.ltype_ == Type::String && op.rtype_ == Type::Int)
    result_type = Type::String;  // str * int
  else if (op.ltype_ == Type::String && op.rtype_ == Type::String) {
    result_type = op.op_ == OperatorCode::Plus ? Type::String /* str+str */
                                               : Type::Int /* str <comp> str */;
  }

  token::Operate2 stmt;
  stmt.dst = add_var(result_type);
  stmt.op = op.op_;
  stmt.rhs = pop(op.rtype_);
  stmt.lhs = pop(op.ltype_);

  stack_.Push(stmt.dst);
  token_append(std::move(stmt));
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

void Parser::Add(lex::Goto g) {
  if (g.cond_ == lex::Goto::Condition::Unconditional)
    token_append(Goto{g.label_});
  else {
    GotoIf stmt;
    stmt.cond = g.cond_ == lex::Goto::Condition::True;
    stmt.label = g.label_;
    stmt.src = pop(Type::Int);

    token_append(std::move(stmt));
  }
}

void Parser::Add(lex::Assign a) {
  token::Assign stmt;
  stmt.src = pop(a.rtype_);
  stmt.dst = stack_.Popelm();
  token_append(std::move(stmt));
}

// void Parser::Add(lex::Namae) { token_append(Name{stack_.Popstr()}); }

// void Parser::Add(lex::Textout t) {
//   token_append(Textout{.kidoku = t.kidoku_, .str = stack_.Popstr()});
// }

std::string Parser::DumpTokens() const {
  std::string result;
  for (size_t i = 0; i < token_.size(); ++i)
    result +=
        std::to_string(i) + ": " +
        std::visit([](const auto& v) { return ToDebugString(v); }, token_[i]) +
        "\n";
  return result;
}

}  // namespace libsiglus
