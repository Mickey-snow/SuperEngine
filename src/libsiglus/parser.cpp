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
#include "libsiglus/callable_builder.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/scene.hpp"
#include "utilities/flat_map.hpp"

#include <sstream>

namespace libsiglus {
using namespace token;

// -----------------------------------------------------------------------
// class Parser
Parser::Parser(Context& ctx) : ctx_(ctx), reader_("") {
  struct ElmParserCtx : public elm::ElementParser::Context {
    libsiglus::Parser& self;
    ElmParserCtx(libsiglus::Parser& s) : self(s) {}

    const std::vector<libsiglus::Property>& SceneProperties() const final {
      return self.ctx_.SceneProperties();
    }
    const std::vector<libsiglus::Property>& GlobalProperties() const final {
      return self.ctx_.GlobalProperties();
    }
    const std::vector<libsiglus::Command>& SceneCommands() const final {
      return self.ctx_.SceneCommands();
    }
    const std::vector<libsiglus::Command>& GlobalCommands() const final {
      return self.ctx_.GlobalCommands();
    }
    const std::vector<Type>& CurcallArgs() const final {
      return self.curcall_args_;
    }

    int ReadKidoku() final { return self.read_kidoku(); }
    int SceneId() const final { return self.ctx_.SceneId(); }
    void Warn(std::string message) final { self.ctx_.Warn(std::move(message)); }
  };

  auto elmctx = std::make_unique<ElmParserCtx>(*this);
  elm_parser_ = std::make_unique<elm::ElementParser>(std::move(elmctx));
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

    case Type::Other:  // ignore
    case Type::Invalid:
    case Type::Callable:
    case Type::None:
      return {};

    default: {
      token::MakeVariable var_tok;
      var_tok.elmcode = stack_.Popelm();
      auto var = add_var(type);
      var_tok.dst = var;
      emit_token(std::move(var_tok));
      return var;  // TODO: return element?
    }
  }
}

Value Parser::pop_arg(const elm::ArgumentList::node_t& node) {
  return std::visit(
      [&](const auto& x) -> Value {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, Type>)
          return pop(x);
        else {  // type is list
          std::vector<Value> vals;
          vals.reserve(x.args.size());
          for (auto it = x.args.crbegin(); it != x.args.crend(); ++it)
            vals.emplace_back(pop_arg(*it));
          return List(std::move(vals));
        }
      },
      node);
}

void Parser::push(const token::GetProperty& prop) {
  Type type = Typeof(prop.dst);

  switch (type) {
    case Type::Int:
    case Type::String:
      push(prop.dst);
      break;

    default:
      push(prop.elmcode);
      break;
  }
}

void Parser::add_label(int id) { emit_token(Label{id}); }

void Parser::ParseAll() {
  reader_ = ByteReader(ctx_.SceneData());
  var_cnt_ = lineno_ = 0;
  offset2cmd_.clear();
  offset2labels_.clear();
  stack_.Clear();

  const int this_scene_id = ctx_.SceneId();
  std::vector<int> const& labels = ctx_.Labels();
  std::vector<libsiglus::Command> const& scene_cmd = ctx_.SceneCommands();
  std::vector<libsiglus::Command> const& global_cmd = ctx_.GlobalCommands();
  for (size_t i = 0; i < labels.size(); ++i)
    offset2labels_.emplace(labels[i], i);  // (location, lid)
  for (size_t i = 0; i < scene_cmd.size(); ++i)
    offset2cmd_.emplace(scene_cmd[i].offset, &scene_cmd[i]);
  for (size_t i = 0; i < global_cmd.size(); ++i)
    if (global_cmd[i].scene_id == this_scene_id)
      offset2cmd_.emplace(global_cmd[i].offset, &global_cmd[i]);

  while (reader_.Position() < reader_.Size()) {
    // Add labels
    for (auto [begin, end] = offset2labels_.equal_range(reader_.Position());
         begin != end; ++begin) {
      add_label(begin->second);
      debug_assert_stack_empty();
    }

    // update curcall
    const auto it = offset2cmd_.find(reader_.Position());
    if (it != offset2cmd_.cend()) {
      curcall_cmd_ = it->second;
      curcall_args_.clear();
      debug_assert_stack_empty();
    }

    try {  // parse
      static Lexer lexer;
      auto lex = lexer.Parse(reader_);
      Add(std::move(lex));
    } catch (std::runtime_error& e) {
      std::string stack_dbgstr = "\nstack:\n" + stack_.ToDebugString();
      throw std::runtime_error(e.what() + std::move(stack_dbgstr));
    }
  }
}

void Parser::Add(lex::Push p) {
  switch (p.type_) {
    case Type::Int:
      push(Integer(p.value_));
      break;
    case Type::String:
      push(String(ctx_.Strings()[p.value_]));
      break;

    default:  // ignore
      break;
  }
}

void Parser::Add(lex::Pop p) { pop(p.type_); }

void Parser::Add(lex::Line line) {
  lineno_ = line.linenum_;
  // is it safe to assume the stack is empty here?
  debug_assert_stack_empty();
}

void Parser::Add(lex::Marker marker) { stack_.PushMarker(); }

void Parser::Add(lex::Property) {
  token::GetProperty tok;
  tok.elmcode = stack_.Popelm();
  tok.chain = elm_parser_->Parse(tok.elmcode);
  tok.dst = add_var(tok.chain.GetType());

  push(tok);
  emit_token(std::move(tok));
}

void Parser::Add(lex::Command command) {
  auto& sig = command.sig;

  token::Command tok;
  elm::Invoke call;

  call.return_type = sig.rettype;
  call.overload_id = sig.overload_id;

  call.named_arg.resize(sig.argtags.size());
  call.arg.resize(sig.arglist.size() - sig.argtags.size());
  for (auto it = call.named_arg.rbegin(); it != call.named_arg.rend(); ++it) {
    it->first = sig.argtags.back();
    it->second = pop_arg(sig.arglist.args.back());
    sig.argtags.pop_back();
    sig.arglist.args.pop_back();
  }
  for (auto it = call.arg.rbegin(); it != call.arg.rend(); ++it) {
    *it = pop_arg(sig.arglist.args.back());
    sig.arglist.args.pop_back();
  }

  tok.elmcode = stack_.Popelm();
  tok.dst = add_var(call.return_type);
  tok.elmcode.ForceBind(std::move(call));
  tok.chain = elm_parser_->Parse(tok.elmcode);

  push(tok.dst);

  emit_token(std::move(tok));
}

void Parser::Add(lex::Operate1 op) {  // + - ~ <int>
  token::Operate1 tok;
  tok.rhs = stack_.Popint();
  tok.dst = add_var(Type::Int);
  tok.op = op.op_;
  tok.val = TryEval(tok.op, tok.rhs);

  push(tok.val.value_or(tok.dst));
  emit_token(std::move(tok));
}

void Parser::Add(lex::Operate2 op) {
  Type result_type = Type::Invalid;

  if (op.ltype_ == Type::Int && op.rtype_ == Type::Int)
    result_type = Type::Int;
  else if (op.ltype_ == Type::String && op.rtype_ == Type::Int)
    result_type = Type::String;  // str * int
  else if (op.ltype_ == Type::String && op.rtype_ == Type::String) {
    result_type = op.op_ == OperatorCode::Plus ? Type::String /* str+str */
                                               : Type::Int /* str <comp> str */;
  }

  token::Operate2 tok;
  tok.dst = add_var(result_type);
  tok.op = op.op_;
  tok.rhs = pop(op.rtype_);
  tok.lhs = pop(op.ltype_);
  tok.val = TryEval(tok.lhs, tok.op, tok.rhs);

  push(tok.val.value_or(tok.dst));
  emit_token(std::move(tok));
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

  push(dst);
  emit_token(Duplicate{std::move(src), std::move(dst)});
}

void Parser::Add(lex::CopyElm) { push(stack_.Backelm()); }

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
  tok.dst = elm_parser_->Parse(tok.dst_elmcode);
  emit_token(std::move(tok));
}

void Parser::Add(lex::Gosub s) {
  token::Gosub tok;

  tok.dst = add_var(s.return_type_);

  std::vector<Value> args;
  args.reserve(s.argt_.size());
  std::transform(s.argt_.args.rbegin(), s.argt_.args.rend(),
                 std::back_inserter(args),
                 [&](const auto& t) { return pop_arg(t); });
  tok.args = std::move(args);
  tok.entry_id = s.label_;

  push(tok.dst);
  emit_token(std::move(tok));
}

void Parser::Add(lex::Arg a) {
  token::Subroutine tok;
  tok.name = curcall_cmd_->name;
  tok.args = curcall_args_;
  emit_token(std::move(tok));
}

void Parser::Add(lex::Return r) {
  token::Return ret;
  ret.ret_vals.reserve(r.ret_types_.size());
  std::transform(r.ret_types_.args.rbegin(), r.ret_types_.args.rend(),
                 std::back_inserter(ret.ret_vals),
                 [&](const auto& t) { return pop_arg(t); });
  emit_token(std::move(ret));
}

void Parser::Add(lex::Declare d) {
  std::ignore = d.size;
  curcall_args_.push_back(d.type);
}

void Parser::Add(lex::Namae) { emit_token(Name{pop(Type::String)}); }

void Parser::Add(lex::Textout t) {
  emit_token(Textout{.kidoku = t.kidoku_, .str = pop(Type::String)});
}

void Parser::Add(lex::EndOfScene) {
  // force parser loop to quit
  reader_.Seek(reader_.Size());

  emit_token(Eof{});
}

void Parser::Add(lex::SelBegin) { ctx_.Warn("selbegin not implemented yet"); }

void Parser::Add(lex::SelEnd) { ctx_.Warn("selend not implemented yet"); }

void Parser::debug_assert_stack_empty() {
  if (!stack_.Empty()) {
    std::ostringstream oss;
    oss << "[Parser] ";
    oss << "at " << ctx_.SceneId() << ':' << ctx_.GetDebugTitle() << '\n';
    oss << "at line " << lineno_ << ", expected stack to be empty. but got:\n";
    oss << stack_.ToDebugString();
    ctx_.Warn(oss.str());

    stack_.Clear();
  }
}

}  // namespace libsiglus
