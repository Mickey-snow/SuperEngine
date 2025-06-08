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
#include "utilities/flat_map.hpp"

namespace libsiglus {
using namespace token;

// -----------------------------------------------------------------------
// class Parser
static DomainLogger logger("Parser");

Parser::Parser(Context& ctx) : ctx_(ctx), reader_("") {}

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
    case Type::Object: {
      ElementCode elmcode = stack_.Popelm();
      Value dst = add_var(Type::Object);
      emit_token(GetProperty{
          .elmcode = elmcode, .chain = resolve_element(elmcode), .dst = dst});
      return dst;
    }
    default:  // ignore
      return {};
  }
}

Value Parser::pop_arg(const ArgumentList::node_t& node) {
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

  for (size_t i = 0; i < ctx_.Labels().size(); ++i)
    offset2labels_.emplace(ctx_.Labels()[i], i);  // (location, lid)
  for (size_t i = 0; i < ctx_.SceneCommands().size(); ++i)
    offset2cmd_.emplace(ctx_.SceneCommands()[i].offset,
                        &ctx_.SceneCommands()[i]);
  for (size_t i = 0; i < ctx_.GlobalCommands().size(); ++i)
    if (ctx_.GlobalCommands()[i].scene_id == ctx_.SceneId())
      offset2cmd_.emplace(ctx_.GlobalCommands()[i].offset,
                          &ctx_.GlobalCommands()[i]);

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
  tok.chain = resolve_element(tok.elmcode);
  tok.dst = add_var(tok.chain.GetType());

  push(tok);
  emit_token(std::move(tok));
}

void Parser::Add(lex::Command command) {
  auto& sig = command.sig;

  token::Command tok;
  tok.return_type = sig.rettype;
  tok.overload_id = sig.overload_id;

  tok.named_arg.resize(sig.argtags.size());
  tok.arg.resize(sig.arglist.size() - sig.argtags.size());
  for (auto it = tok.named_arg.rbegin(); it != tok.named_arg.rend(); ++it) {
    it->first = sig.argtags.back();
    it->second = pop_arg(sig.arglist.args.back());
    sig.argtags.pop_back();
    sig.arglist.args.pop_back();
  }
  for (auto it = tok.arg.rbegin(); it != tok.arg.rend(); ++it) {
    *it = pop_arg(sig.arglist.args.back());
    sig.arglist.args.pop_back();
  }

  tok.elmcode = stack_.Popelm();
  tok.chain = resolve_element(tok.elmcode);

  tok.dst = add_var(tok.return_type);
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
  tok.dst = resolve_element(tok.dst_elmcode);
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

void Parser::debug_assert_stack_empty() {
  if (!stack_.Empty()) {
    auto rec = logger(Severity::Info);
    rec << "at " << ctx_.GetDebugTitle() << '\n';
    rec << "at line " << lineno_ << ", expected stack to be empty. but got:\n";
    rec << stack_.ToDebugString();
    stack_.Clear();
  }
}

// -----------------------------------------------------------------------
// element related
elm::AccessChain Parser::resolve_element(const ElementCode& elmcode) {
  auto elm = elmcode.IntegerView();

  const auto flag = (elm.front() >> 24) & 0xFF;
  size_t idx = elm.front() ^ (flag << 24);
  if (flag == USER_COMMAND_FLAG)
    return resolve_usrcmd(elmcode, idx);
  else if (flag == USER_PROPERTY_FLAG)
    return resolve_usrprop(elmcode, idx);

  else  // built-in element
    return make_element(elmcode);
}

elm::AccessChain Parser::resolve_usrcmd(const ElementCode& elmcode,
                                        size_t idx) {
  auto chain = elm::AccessChain();

  const libsiglus::Command* cmd = nullptr;
  if (idx < ctx_.GlobalCommands().size())
    cmd = &ctx_.GlobalCommands()[idx];
  else
    cmd = &ctx_.SceneCommands()[idx - ctx_.GlobalCommands().size()];

  if (cmd->name.starts_with("$$pos_convert")) {
    [[maybe_unused]]

    volatile int dummy = -1;
  }
  chain.root = elm::Usrcmd{
      .scene = cmd->scene_id, .entry = cmd->offset, .name = cmd->name};
  // return type?
  return chain;
}

elm::AccessChain Parser::resolve_usrprop(const ElementCode& elmcode,
                                         size_t idx) {
  elm::Usrprop root;
  Type root_type;

  if (idx < ctx_.GlobalProperties().size()) {
    const auto& incprop = ctx_.GlobalProperties()[idx];
    root.name = incprop.name;
    root_type = incprop.form;
    root.scene = -1;  // global
    root.idx = idx;
  } else {
    idx -= ctx_.GlobalProperties().size();
    const auto& usrprop = ctx_.SceneProperties()[idx];
    root.name = usrprop.name;
    root_type = usrprop.form;
    root.scene = ctx_.SceneId();
    root.idx = idx;
  }

  return elm::make_chain(root_type, std::move(root), elmcode, 1);
}

elm::AccessChain Parser::make_element(const ElementCode& elmcode) {
  auto elm = elmcode.IntegerView();
  int root = elm.front();

  switch (root) {
    case 25:  // A
      return elm::make_chain(Type::IntList, elm::Sym("A"), elmcode, 1);
    case 26:  // B
      return elm::make_chain(Type::IntList, elm::Sym("B"), elmcode, 1);
    case 27:  // C
      return elm::make_chain(Type::IntList, elm::Sym("C"), elmcode, 1);
    case 28:  // D
      return elm::make_chain(Type::IntList, elm::Sym("D"), elmcode, 1);
    case 29:  // E
      return elm::make_chain(Type::IntList, elm::Sym("E"), elmcode, 1);
    case 30:  // F
      return elm::make_chain(Type::IntList, elm::Sym("F"), elmcode, 1);
    case 137:  // X
      return elm::make_chain(Type::IntList, elm::Sym("X"), elmcode, 1);
    case 31:  // G
      return elm::make_chain(Type::IntList, elm::Sym("G"), elmcode, 1);
    case 32:  // Z
      return elm::make_chain(Type::IntList, elm::Sym("Z"), elmcode, 1);

    case 34:  // S
      return elm::make_chain(Type::StrList, elm::Sym("S"), elmcode, 1);
    case 35:  // M
      return elm::make_chain(Type::StrList, elm::Sym("M"), elmcode, 1);
    case 106:  // NAMAE_LOCAL
      return elm::make_chain(Type::StrList, elm::Sym("LN"), elmcode, 1);
    case 107:  // NAMAE_GLOBAL
      return elm::make_chain(Type::StrList, elm::Sym("GN"), elmcode, 1);

    case 5:  // FARCALL
      return elm::make_chain(Type::Callable, elm::Sym("farcall"), elmcode, 1);

    case 74:  // SET_TITLE
      return elm::make_chain(Type::Callable, elm::Sym("set_title"), elmcode, 1);

    case 75:  // GET_TITLE
      return elm::make_chain(Type::Callable, elm::Sym("get_title"), elmcode, 1);

    case 83: {  // CUR_CALL
      const int elmcall = elm[1];
      if ((elmcall >> 24) == 0x7d) {
        auto id = (elmcall ^ (0x7d << 24));
        return elm::make_chain(curcall_args_[id], elm::Arg(id), elmcode, 2);
      }

      else if (elmcall == 0)
        return elm::make_chain(Type::IntList, elm::Sym("L"), elmcode, 2);

      else if (elmcall == 1)
        return elm::make_chain(Type::StrList, elm::Sym("K"), elmcode, 2);
    } break;

    case 18: {  // KOE
      read_kidoku();
      break;
    }

    case 76:   // SELBTN
    case 77:   // SELBTN_READY
    case 126:  // SELBTN_CANCEL
    case 128:  // SELBTN_CANCEL_READY
    case 127:  // SELBTN_START
    {
      if (root == 76 || root == 126 || root == 127)
        read_kidoku();
      break;
    }

    case 92:  // SYSTEM
      return elm::make_chain(Type::System, elm::Sym("os"), elmcode, 1);

    case 40:  // COUNTER
      return elm::make_chain(Type::CounterList, elm::Sym("counter"), elmcode,
                             1);

    case 79:  // FRAME_ACTION
      return elm::make_chain(Type::FrameAction, elm::Sym("frame_action"),
                             elmcode, 1);
    case 53:  // FRAME_ACTION_CH
      return elm::make_chain(Type::FrameActionList, elm::Sym("frame_action_ch"),
                             elmcode, 1);

    case 12:  // MWND_PRINT
      return elm::AccessChain{
          .root = {Type::Callable, elm::Print(read_kidoku())}, .nodes = {}};

    case 49:  // STAGE
      return elm::make_chain(Type::StageList, elm::Sym("stage"), elmcode, 1);
    case 37:  // BACK
      return elm::make_chain(Type::Stage, elm::Sym("stage_back"), elmcode, 1);
    case 38:  // FRONT
      return elm::make_chain(Type::Stage, elm::Sym("stage_front"), elmcode, 1);
    case 73:  // NEXT
      return elm::make_chain(Type::Stage, elm::Sym("stage_next"), elmcode, 1);

    case 65:  // EXCALL
      return elm::make_chain(Type::Excall, elm::Sym("excall"), elmcode, 1);

    case 135:  // MASK
      return elm::make_chain(Type::MaskList, elm::Sym("mask"), elmcode, 1);

    default:
      break;
  }

  elm::AccessChain uke;
  uke.root = elm::Sym('<' + ToString(elmcode.code.front()) + '>');
  uke.nodes.reserve(elmcode.code.size() - 1);
  for (size_t i = 1; i < elmcode.code.size(); ++i)
    uke.nodes.emplace_back(elm::Val(elmcode.code[i]));
  return uke;
}

}  // namespace libsiglus
