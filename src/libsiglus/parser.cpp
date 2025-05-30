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

#include "libsiglus/elm_details/curcall.hpp"
#include "libsiglus/elm_details/farcall.hpp"
#include "libsiglus/elm_details/koe.hpp"
#include "libsiglus/elm_details/memory.hpp"
#include "libsiglus/elm_details/selbtn.hpp"
#include "libsiglus/elm_details/title.hpp"

namespace libsiglus {
using namespace token;

// -----------------------------------------------------------------------
// class Parser
static DomainLogger logger("Parser");

Parser::Parser(Archive& archive,
               Scene& scene,
               std::shared_ptr<OutputBuffer> out)
    : archive_(archive),
      scene_(scene),
      out_(out),
      reader_(scene_.scene_),
      lineno_(0),
      var_cnt_(0) {
  if (out_ == nullptr) {
    class Dummy : public OutputBuffer {
     public:
      void operator=(token::Token_t) override {}
    };
    out_ = std::make_shared<Dummy>();
  }

  for (size_t i = 0; i < scene.label.size(); ++i) {
    offset2labels_.emplace(scene.label[i], i);  // (location, lid)
  }

  for (size_t i = 0; i < scene.cmd.size(); ++i) {
    offset2cmd_.emplace(scene.cmd[i].offset, i);
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

void Parser::add_label(int id) { emit_token(Label{id}); }

void Parser::ParseAll() {
  while (reader_.Position() < reader_.Size()) {
    // Add labels
    for (auto [begin, end] = offset2labels_.equal_range(reader_.Position());
         begin != end; ++begin) {
      add_label(begin->second);
    }

    try {  // parse
      static Lexer lexer;
      std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
                 lexer.Parse(reader_));
    } catch (std::runtime_error& e) {
      std::string stack_dbgstr = "\nstack:\n" + stack_.ToDebugString();
      throw std::runtime_error(e.what() + std::move(stack_dbgstr));
    }
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
    logger(Severity::Info) << "at line " << lineno_
                           << "expected stack to be empty. but got:\n"
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
  emit_token(std::move(tok));
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
  tok.val = TryEval(tok.op, tok.rhs);

  stack_.Push(tok.val.value_or(tok.dst));
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

  stack_.Push(tok.val.value_or(tok.dst));
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

void Parser::Add(lex::Gosub s) {
  token::Gosub tok;

  tok.dst = add_var(s.return_type_);

  std::vector<Value> args;
  args.reserve(s.argt_.size());
  std::transform(s.argt_.rbegin(), s.argt_.rend(), std::back_inserter(args),
                 [&](const Type& t) { return pop(t); });
  tok.args = std::move(args);
  tok.entry_id = s.label_;

  emit_token(std::move(tok));
}

void Parser::Add(lex::Arg a) {
  int cmd_id = offset2cmd_.lower_bound(reader_.Position())->second;
  token ::Subroutine tok;
  tok.name = scene_.cmd[cmd_id].name;
  emit_token(std::move(tok));
}

void Parser::Add(lex::Return r) {
  token::Return ret;
  ret.ret_vals.reserve(r.ret_types_.size());
  std::transform(r.ret_types_.rbegin(), r.ret_types_.rend(),
                 std::back_inserter(ret.ret_vals),
                 [&](const Type& t) { return pop(t); });
  emit_token(std::move(ret));
}

void Parser::Add(lex::Declare d) {
  token::Precall pre;
  pre.type = d.type;
  pre.size = d.size;
  emit_token(std::move(pre));
}

void Parser::Add(lex::Namae) { emit_token(Name{pop(Type::String)}); }

void Parser::Add(lex::Textout t) {
  emit_token(Textout{.kidoku = t.kidoku_, .str = pop(Type::String)});
}

// element related
Element Parser::resolve_element(const ElementCode& elmcode) {
  auto elm = elmcode.IntegerView();

  const auto flag = (elm.front() >> 24) & 0xFF;
  size_t idx = elm.front() ^ (flag << 24);

  if (flag == USER_COMMAND_FLAG) {
    auto result = std::make_unique<UserCommand>();
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

  } else if (elm[0] == 83 && elm[1] == 2097152000) {
    // hack
    struct CallArg : public IElement {
      CallArg() : IElement(Type::Int) {}
      int id = 0;
      elm::Kind Kind() const noexcept override { return elm::Kind::Callprop; }
      std::string ToDebugString() const override {
        return "arg_" + std::to_string(id);
      }
    };
    return std::make_unique<CallArg>();

  } else {  // built-in element
    return make_element(elmcode);
  }
}

Element Parser::make_element(const ElementCode& elmcode) {
  auto elm = elmcode.IntegerView();

  enum Global {
    A = 25,
    B = 26,
    C = 27,
    D = 28,
    E = 29,
    F = 30,
    X = 137,
    G = 31,
    Z = 32,
    S = 34,
    M = 35,
    NAMAE_GLOBAL = 107,
    NAMAE_LOCAL = 106,
    NAMAE = 108,
    STAGE = 49,
    BACK = 37,
    FRONT = 38,
    NEXT = 73,
    MSGBK = 145,
    SCREEN = 70,
    COUNTER = 40,
    FRAME_ACTION = 79,
    FRAME_ACTION_CH = 53,
    TIMEWAIT = 54,
    TIMEWAIT_KEY = 55,
    MATH = 39,
    DATABASE = 105,
    CGTABLE = 78,
    BGMTABLE = 123,
    G00BUF = 124,
    MASK = 135,
    EDITBOX = 97,
    FILE = 48,
    BGM = 42,
    KOE_ST = 82,
    PCM = 43,
    PCMCH = 44,
    PCMEVENT = 52,
    SE = 45,
    MOV = 20,
    INPUT = 86,
    MOUSE = 46,
    KEY = 24,
    SCRIPT = 64,
    SYSCOM = 63,
    SYSTEM = 92,
    CALL = 98,
    CUR_CALL = 83,
    EXCALL = 65,
    STEAM = 166,
    EXKOE = 87,
    EXKOE_PLAY_WAIT = 88,
    EXKOE_PLAY_WAIT_KEY = 89,
    NOP = 60,
    OWARI = 2,
    RETURNMENU = 3,
    JUMP = 4,
    FARCALL = 5,
    GET_SCENE_NAME = 131,
    GET_LINE_NO = 158,
    SET_TITLE = 74,
    GET_TITLE = 75,
    SAVEPOINT = 36,
    CLEAR_SAVEPOINT = 113,
    CHECK_SAVEPOINT = 112,
    SELPOINT = 1,
    CLEAR_SELPOINT = 111,
    CHECK_SELPOINT = 110,
    STACK_SELPOINT = 149,
    DROP_SELPOINT = 150,
    DISP = 96,
    FRAME = 6,
    CAPTURE = 80,
    CAPTURE_FROM_FILE = 163,
    CAPTURE_FREE = 81,
    CAPTURE_FOR_OBJECT = 130,
    CAPTURE_FOR_OBJECT_FREE = 136,
    CAPTURE_FOR_TWEET = 164,
    CAPTURE_FREE_FOR_TWEET = 165,
    CAPTURE_FOR_LOCAL_SAVE = 144,
    WIPE = 7,
    WIPE_ALL = 23,
    MASK_WIPE = 50,
    MASK_WIPE_ALL = 51,
    WIPE_END = 33,
    WAIT_WIPE = 103,
    CHECK_WIPE = 109,
    MESSAGE_BOX = 0,
    SET_MWND = 8,
    GET_MWND = 116,
    SET_SEL_MWND = 71,
    GET_SEL_MWND = 117,
    SET_WAKU = 22,
    OPEN = 9,
    OPEN_WAIT = 58,
    OPEN_NOWAIT = 59,
    CLOSE = 10,
    CLOSE_WAIT = 56,
    CLOSE_NOWAIT = 57,
    END_CLOSE = 125,
    MSG_BLOCK = 84,
    MSG_PP_BLOCK = 121,
    CLEAR = 11,
    SET_NAMAE = 156,
    PRINT = 12,
    RUBY = 61,
    MSGBTN = 47,
    NL = 15,
    NLI = 62,
    INDENT = 119,
    CLEAR_INDENT = 94,
    WAIT_MSG = 21,
    PP = 13,
    R = 14,
    PAGE = 115,
    REP_POS = 151,
    SIZE = 16,
    COLOR = 17,
    MULTI_MSG = 95,
    NEXT_MSG = 93,
    START_SLIDE_MSG = 120,
    END_SLIDE_MSG = 122,
    KOE = 18,
    KOE_PLAY_WAIT = 90,
    KOE_PLAY_WAIT_KEY = 91,
    KOE_SET_VOLUME = 152,
    KOE_SET_VOLUME_MAX = 153,
    KOE_SET_VOLUME_MIN = 154,
    KOE_GET_VOLUME = 155,
    KOE_STOP = 68,
    KOE_WAIT = 85,
    KOE_WAIT_KEY = 99,
    KOE_CHECK = 69,
    KOE_CHECK_GET_KOE_NO = 159,
    KOE_CHECK_GET_CHARA_NO = 160,
    KOE_CHECK_IS_EX_KOE = 161,
    CLEAR_FACE = 72,
    SET_FACE = 41,
    CLEAR_MSGBK = 118,
    INSERT_MSGBK_IMG = 114,
    SEL = 19,
    SEL_CANCEL = 101,
    SELMSG = 100,
    SELMSG_CANCEL = 102,
    SELBTN = 76,
    SELBTN_READY = 77,
    SELBTN_CANCEL = 126,
    SELBTN_CANCEL_READY = 128,
    SELBTN_START = 127,
    GET_LAST_SEL_MSG = 162,
    INIT_CALL_STACK = 141,
    DEL_CALL_STACK = 138,
    SET_CALL_STACK_CNT = 140,
    GET_CALL_STACK_CNT = 139,
    __FOG_NAME = 132,
    __FOG_X = 142,
    __FOG_X_EVE = 143,
    __FOG_NEAR = 133,
    __FOG_FAR = 134,
    __TEST = 104
  };

  int root = elm.front();
  switch (root) {
    case A:
    case B:
    case C:
    case D:
    case E:
    case F:
    case X:
    case G:
    case Z:
    case S:
    case M:
    case NAMAE_LOCAL:
    case NAMAE_GLOBAL:
      return elm::Memory::Parse(elmcode);

    case FARCALL:
      return std::make_unique<elm::Farcall>();

    case SET_TITLE:
      return std::make_unique<elm::SetTitle>();
    case GET_TITLE:
      return std::make_unique<elm::GetTitle>();

    case CUR_CALL: {
      auto result = std::make_unique<elm::Curcall>();
      if ((elm[1] >> 24) == 0x7d) {  // hack
        result->type = Type::Int;
        result->id = elm[1] ^ (0x7d << 24);
      }
      return result;
    }

    case KOE: {
      auto result = std::make_unique<elm::Koe>();
      result->kidoku = reader_.PopAs<int>(4);
      return result;
    }

    case SELBTN:
    case SELBTN_READY:
    case SELBTN_CANCEL:
    case SELBTN_CANCEL_READY:
    case SELBTN_START: {
      auto result = std::make_unique<elm::Selbtn>();
      if (root == SELBTN || root == SELBTN_CANCEL || root == SELBTN_START)
        result->kidoku = reader_.PopAs<int>(4);
      return result;
    }

    default:
      break;
  }

  auto uke = std::make_unique<UnknownElement>();
  uke->elmcode = elmcode;
  return uke;
}

}  // namespace libsiglus
