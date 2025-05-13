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

namespace libsiglus {
using namespace token;

// -----------------------------------------------------------------------
// class Parser
static DomainLogger logger("Parser");

Parser::Parser(Archive& archive, Scene& scene)
    : archive_(archive),
      scene_(scene),
      reader_(scene_.scene_),
      lineno_(0),
      var_cnt_(0) {
  for (size_t i = 0; i < scene.label.size(); ++i) {
    offset2labels_.emplace(scene.label[i], i);  // (location, lid)
  }
}

Value Parser::add_var(Type type) {
  Variable var(type, var_cnt_++);
  return var;
}

Value Parser::pop(Type type) {
  if (stack_.Empty())
    return {};

  switch (type) {
    case Type::Int:
      return stack_.Popint();
    case Type::String:
      return stack_.Popstr();
    default:  // ignore
      return {};
  }
}

void Parser::add_label(int id) {
  label2offset_[id] = token_.size();
  emit_token(Label{id});
}

Element Parser::resolve_element(std::span<int> elm) const {
  const auto flag = (elm.front() >> 24) & 0xFF;
  size_t idx = elm.front() ^ (flag << 24);

  if (flag == USER_COMMAND_FLAG) {
    auto result = std::make_unique<UserCommand>();

    result->root_id = elm.front();
    result->type = Type::Other;

    libsiglus::Command* cmd = nullptr;
    if (idx < archive_.cmd_.size())
      cmd = archive_.cmd_.data() + idx;
    else
      cmd = scene_.cmd.data() + (idx - archive_.cmd_.size());

    result->name = cmd->name;
    result->scene = cmd->scene_id;
    result->entry = cmd->offset;
    return result;

  } else if (flag == USER_PROPERTY_FLAG) {
    auto result = std::make_unique<UserProperty>();
    result->root_id = elm.front();

    if (idx < archive_.prop_.size()) {
      const auto& incprop = archive_.prop_[idx];
      result->name = incprop.name;
      result->type = incprop.form;
      result->scene = -1;
      result->idx = idx;
    } else {
      idx -= archive_.prop_.size();
      const auto& usrprop = scene_.property[idx];
      result->name = usrprop.name;
      result->type = usrprop.form;
      result->scene = scene_.id_;
      result->idx = idx;
    }
    return result;

  } else {  // built-in element
    int root = elm.front();
    std::span<int> path = elm.subspan(1);
    return MakeElement(root, path);
  }
}

void Parser::ParseAll() {
  while (reader_.Position() < reader_.Size()) {
    // Add labels
    for (auto [begin, end] = offset2labels_.equal_range(reader_.Position());
         begin != end; ++begin) {
      add_label(begin->second);
    }

    // is it safe to assume the stack is empty here?
    if (offset2labels_.contains(reader_.Position()) && !stack_.Empty())
      logger(Severity::Warn) << stack_.ToDebugString();

    // parse
    static Lexer lexer;
    std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
               lexer.Parse(reader_));
  }
}

void Parser::Add(lex::Push push) {
  switch (push.type_) {
    case Type::Int:
      stack_.Push(Integer(push.value_));
      break;
    case Type::String:
      stack_.Push(String(scene_.str_[push.value_]));
      break;

    default:  // ignore
      break;
  }
}

void Parser::Add(lex::Pop p) { pop(p.type_); }

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
  token::GetProperty tok;
  tok.elmcode = stack_.Popelm();
  tok.elm = resolve_element(tok.elmcode);
  tok.dst = add_var(tok.elm->type);

  stack_.Push(tok.dst);
  token_.emplace_back(std::move(tok));
}

void Parser::Add(lex::Command command) {
  token::Command tok;
  tok.return_type = command.rettype_;
  tok.overload_id = command.override_;

  tok.named_arg.resize(command.arg_tag_.size());
  tok.arg.resize(command.argt_.size() - command.arg_tag_.size());
  for (auto it = tok.named_arg.rbegin(); it != tok.named_arg.rend(); ++it) {
    it->first = command.arg_tag_.back();
    it->second = pop(command.argt_.back());
    command.arg_tag_.pop_back();
    command.argt_.pop_back();
  }
  for (auto it = tok.arg.rbegin(); it != tok.arg.rend(); ++it) {
    *it = pop(command.argt_.back());
    command.argt_.pop_back();
  }

  tok.elmcode = stack_.Popelm();
  tok.elm = resolve_element(tok.elmcode);

  tok.dst = add_var(tok.return_type);
  stack_.Push(tok.dst);

  emit_token(std::move(tok));
}

void Parser::Add(lex::Operate1 op) {  // + - ~ <int>
  token::Operate1 tok;
  tok.rhs = stack_.Popint();
  tok.dst = add_var(Type::Int);
  tok.op = op.op_;

  stack_.Push(tok.dst);
  emit_token(std::move(tok));
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
  emit_token(std::move(stmt));
}

void Parser::Add(lex::Copy cp) {
  Value src, dst = add_var(cp.type_);

  switch (cp.type_) {
    case Type::Int:
      src = stack_.Backint();
      break;
    case Type::String:
      src = stack_.Backstr();
      break;

    default:
      break;
  }

  stack_.Push(dst);
  emit_token(Duplicate{std::move(src), std::move(dst)});
}

void Parser::Add(lex::CopyElm) { stack_.Push(stack_.Backelm()); }

void Parser::Add(lex::Goto g) {
  if (g.cond_ == lex::Goto::Condition::Unconditional)
    emit_token(Goto{g.label_});
  else {
    GotoIf tok;
    tok.cond = g.cond_ == lex::Goto::Condition::True;
    tok.label = g.label_;
    tok.src = pop(Type::Int);

    emit_token(std::move(tok));
  }
}

void Parser::Add(lex::Assign a) {
  token::Assign tok;
  tok.src = pop(a.rtype_);
  tok.dst_elmcode = stack_.Popelm();
  tok.dst = resolve_element(tok.dst_elmcode);
  emit_token(std::move(tok));
}

// void Parser::Add(lex::Namae) { token_append(Name{stack_.Popstr()}); }

// void Parser::Add(lex::Textout t) {
//   token_append(Textout{.kidoku = t.kidoku_, .str = stack_.Popstr()});
// }

std::string Parser::DumpTokens() const {
  std::string result;
  for (size_t i = 0; i < token_.size(); ++i)
    result += std::to_string(i) + ": " + ToDebugString(token_[i]) + '\n';
  return result;
}

}  // namespace libsiglus
