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

#include "libsiglus/element_parser.hpp"

#include "libsiglus/callable_builder.hpp"
#include "utilities/flat_map.hpp"
#include "utilities/string_utilities.hpp"

#include <sstream>

namespace libsiglus::elm {
using namespace libsiglus::elm::callable_builder;

// ==============================================================================
// Helpers
class Builder {
 public:
  struct Ctx {
    ElementCode& elm;
    std::span<const Value>& elmcode;
    AccessChain& chain;
    std::function<void(std::string)> Warn;
  };

  Builder(std::function<void(Ctx&)> a) : action_(std::move(a)) {}
  void Build(Ctx& ctx) const { std::invoke(action_, ctx); }

 private:
  std::function<void(Ctx&)> action_;
};

// -----------------------------------------------------------------------
inline static auto b(Type type, Node::var_t node) {
  return Builder([t = type, n = std::move(node)](Builder::Ctx& ctx) {
    ctx.chain.nodes.emplace_back(t, n);
    ctx.elmcode = ctx.elmcode.subspan(1);
  });
}
inline static Builder b_index_array(Type value_type) {
  return Builder([t = value_type](Builder::Ctx& ctx) {
    Subscript subscript{Integer(-1)};
    if (ctx.elmcode.size() < 2) {
      ctx.Warn("[IndexArray] expected index");
      ctx.chain.nodes.emplace_back(t, std::move(subscript));
      ctx.elmcode = ctx.elmcode.subspan(ctx.elmcode.size());
      return;
    }

    subscript.idx = ctx.elmcode[1];
    ctx.chain.nodes.emplace_back(t, std::move(subscript));
    ctx.elmcode = ctx.elmcode.subspan(2);
  });
}
template <typename... Ts>
inline static Builder callable(Ts&&... params) {
  auto builder = Builder([callable = make_callable(
                              std::forward<Ts>(params)...)](Builder::Ctx& ctx) {
    auto& bind = ctx.elm.bind_ctx;
    auto& chain = ctx.chain;

    // consume the callable element
    ctx.elmcode = ctx.elmcode.subspan(1);

    // resolve overload
    auto candidate = std::find_if(
        callable.overloads.begin(), callable.overloads.end(),
        [overload = bind.overload_id](const Function& fn) {
          return !fn.overload.has_value() || *fn.overload == overload;
        });
    if (candidate == callable.overloads.end()) {
      std::ostringstream oss;
      oss << "[Callable] ";
      oss << "Overload " << std::to_string(bind.overload_id) << " not found in "
          << callable.ToDebugString() << '\n';
      oss << "access chain: " << chain.ToDebugString();
      ctx.Warn(oss.str());
    }

    // TODO: Check signature mismatch
    Call call;
    call.args = std::move(bind.arg);
    call.kwargs = std::move(bind.named_arg);
    call.name = candidate->name;

    ctx.elm.force_bind = false;
    chain.nodes.emplace_back(candidate->return_t, std::move(call));

    // check return type
    if (ctx.elm.force_bind && bind.return_type != chain.GetType()) {
      std::ostringstream oss;
      oss << "[Callable] ";
      oss << "return type mismatch: " << ToString(bind.return_type) << " vs "
          << ToString(chain.GetType()) << '\n';
      oss << chain.ToDebugString();
      ctx.Warn(oss.str());
    }
  });
  return builder;
}
static Builder obj_getset(std::string_view mem,
                          Type type,
                          int get_code = 0,
                          int set_code = 1) {
  return callable(fn(mem)[get_code]().ret(type),
                  fn(mem)[set_code](type).ret(Type::None));
}

static Builder obj_getter(std::string_view mem,
                          Type rettype = Type::Int,
                          Type argtype = Type::Int) {
  return callable(fn(mem)[0]().ret(rettype), fn(mem)[1](argtype).ret(rettype),
                  fn(mem)[any]().ret(rettype));
}
static Builder obj_colgetter(std::string_view mem,
                             Type rettype = Type::Int,
                             Type argtype = Type::Int) {
  return callable(fn(mem)[0](argtype, argtype).ret(rettype),
                  fn(mem)[1](argtype, argtype, argtype).ret(rettype),
                  fn(mem)[any]().ret(rettype));
}

// ==============================================================================
// Method map
static flat_map<Builder> const* GetMethodMap(Type type) {
  switch (type) {
    case Type::IntList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | b_index_array(Type::Int),
          id[3] | b(Type::IntList, Member("b1")),
          id[4] | b(Type::IntList, Member("b2")),
          id[5] | b(Type::IntList, Member("b4")),
          id[7] | b(Type::IntList, Member("b8")),
          id[6] | b(Type::IntList, Member("b16")),
          id[10] | b(Type::None, Call("init")),
          id[2] | callable(fn("resize")[any](Type::Int)),
          id[9] | callable(fn("size")[any]().ret(Type::Int)),
          id[8] | callable(fn("fill")[0](Type::Int, Type::Int),
                           fn("fill")[any](Type::Int, Type::Int, Type::Int)),
          id[1] | callable(fn("Set")[any](Type::Int, va_arg(Type::Int))));
      return &mp;
    }

    case Type::IntEventList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::IntEvent),
                                id[1] | callable(fn("resize")[any](Type::Int)));
      return &mp;
    }
    case Type::IntEvent: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | callable(fn("set")[any](Type::Int, Type::Int, Type::Int,
                                          Type::Int, kw_arg(0, Type::Int))),
          id[7] |
              callable(fn("set_real")[any](Type::Int, Type::Int, Type::Int,
                                           Type::Int, kw_arg(0, Type::Int))),
          id[1] | callable(fn("loop")[any](Type::Int, Type::Int, Type::Int,
                                           Type::Int, Type::Int)),
          id[8] | callable(fn("loop_real")[any](Type::Int, Type::Int, Type::Int,
                                                Type::Int, Type::Int)),
          id[2] | callable(fn("turn")[any](Type::Int, Type::Int, Type::Int,
                                           Type::Int, Type::Int)),
          id[9] | callable(fn("turn_real")[any](Type::Int, Type::Int, Type::Int,
                                                Type::Int, Type::Int)),
          id[3] | b(Type::None, Call("end")),
          id[4] | b(Type::None, Call("wait")),
          id[10] | b(Type::None, Call("wait_key")),
          id[5] | b(Type::Int, Call("check")));

      return &mp;
    }

    case Type::StrList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(
                Type::String, Call("substr", {ctx.elmcode[1]}));  // really?
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
          id[3] | b(Type::None, Call("init")),
          id[2] | callable(fn("resize")[any](Type::Int)),
          id[4] | b(Type::Int, Call("size")));
      return &mp;
    }
    case Type::String: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | b(Type::String, Call("upper")),
          id[1] | b(Type::String, Call("lower")),
          id[6] | b(Type::Int, Call("cnt")), id[5] | b(Type::Int, Call("len")),
          id[2] | callable(fn("left")[any](Type::Int).ret(Type::String)),
          id[7] | callable(fn("left_len")[any](Type::Int).ret(Type::String)),
          id[4] | callable(fn("right")[any](Type::Int).ret(Type::String)),
          id[9] | callable(fn("right_len")[any](Type::Int).ret(Type::String)),
          id[3] |
              callable(fn("mid")[0](Type::Int).ret(Type::String),
                       fn("mid")[any](Type::Int, Type::Int).ret(Type::String)),
          id[8] |
              callable(
                  fn("mid_len")[any](Type::Int, Type::Int).ret(Type::String)),
          id[10] | callable(fn("find")[any](Type::String).ret(Type::Int)),
          id[11] | callable(fn("rfind")[any](Type::String).ret(Type::Int)),
          id[13] | callable(fn("charat")[any](Type::Int).ret(Type::Int)),
          id[13] | b(Type::Int, Call("tonum")));
      return &mp;
    }

    case Type::Math: {
      static const auto mp = make_flatmap<Builder>(
          id[3] | callable(fn("max")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[4] | callable(fn("min")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[10] | callable(fn("limit")[any](Type::Int, Type::Int, Type::Int)
                                .ret(Type::Int)),
          id[5] | callable(fn("abs")[any](Type::Int).ret(Type::Int)),
          id[0] |
              callable(fn("rand")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[14] |
              callable(fn("sqrt")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[19] |
              callable(fn("log")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[20] |
              callable(fn("log2")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[21] |
              callable(fn("log10")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[6] | callable(fn("sin")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[7] | callable(fn("cos")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[8] | callable(fn("tan")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[16] |
              callable(fn("asin")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[17] |
              callable(fn("acos")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[18] |
              callable(fn("atan")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[15] | callable(fn("distance")[any](Type::Int, Type::Int, Type::Int,
                                                Type::Int)
                                .ret(Type::Int)),
          id[22] | callable(fn("angle")[any](Type::Int, Type::Int, Type::Int,
                                             Type::Int)
                                .ret(Type::Int)),
          id[9] | callable(fn("linear")[any](Type::Int, Type::Int, Type::Int,
                                             Type::Int, Type::Int)
                               .ret(Type::Int)),
          id[2] | callable(fn("timetable")[any](Type::Int, Type::Int, Type::Int,
                                                va_arg(Type::List))
                               .ret(Type::Int)),
          id[1] |
              callable(fn("tostr")[1](Type::Int, Type::Int).ret(Type::String),
                       fn("tostr")[0](Type::Int).ret(Type::String)),
          id[11] | callable(fn("tostr_zero")[any](Type::Int, Type::Int)
                                .ret(Type::String)),
          id[12] |
              callable(
                  fn("tostr_zen")[0](Type::Int).ret(Type::String),
                  fn("tostr_zen")[1](Type::Int, Type::Int).ret(Type::String)),
          id[13] | callable(fn("tostr_zen_zero")[any](Type::Int, Type::Int)
                                .ret(Type::String)),
          id[23] |
              callable(fn("tostr_code")[any](Type::Int).ret(Type::String)));
      return &mp;
    }

    case Type::System: {
      static const auto mp = make_flatmap<Builder>(
          id[14] | b(Type::None, Member("calendar")),
          id[15] | b(Type::Int, Member("time")),
          id[0] | b(Type::Int, Call("window_active")),
          id[13] | b(Type::Int, Call("is_debug")),
          id[1] | callable(fn("shell_openfile")[any](Type::String)),
          id[5] | callable(fn("openurl")[any](Type::String)),
          id[6] | callable(
                      fn("check_file_exist")[any](Type::String).ret(Type::Int)),
          id[12] |
              callable(
                  fn("check_file_exist")[any](Type::String).ret(Type::Int)),
          id[2] | callable(fn("check_dummy")[any](Type::String, Type::Int,
                                                  Type::String)),
          id[21] | b(Type::None, Call("clear_dummy")),
          id[17] | callable(fn("msgbox_ok")[0](Type::Int).ret(Type::Int),
                            fn("msgbox_ok")[1](Type::String).ret(Type::Int)),
          id[18] |
              callable(fn("msgbox_okcancel")[0](Type::Int).ret(Type::Int),
                       fn("msgbox_okcancel")[1](Type::String).ret(Type::Int)),
          id[19] | callable(fn("msgbox_yn")[0](Type::Int).ret(Type::Int),
                            fn("msgbox_yn")[1](Type::String).ret(Type::Int)),
          id[20] |
              callable(fn("msgbox_yncancel")[0](Type::Int).ret(Type::Int),
                       fn("msgbox_yncancel")[1](Type::String).ret(Type::Int)),
          id[7] |
              callable(fn("debug_msgbox_ok")[0](Type::Int).ret(Type::Int),
                       fn("debug_msgbox_ok")[1](Type::String).ret(Type::Int)),
          id[8] |
              callable(
                  fn("debug_msgbox_okcancel")[0](Type::Int).ret(Type::Int),
                  fn("debug_msgbox_okcancel")[1](Type::String).ret(Type::Int)),
          id[9] |
              callable(fn("debug_msgbox_yn")[0](Type::Int).ret(Type::Int),
                       fn("debug_msgbox_yn")[1](Type::String).ret(Type::Int)),
          id[10] |
              callable(
                  fn("debug_msgbox_yncancel")[0](Type::Int).ret(Type::Int),
                  fn("debug_msgbox_yncancel")[1](Type::String).ret(Type::Int)),

          id[11] | callable(fn("debug_write_log")[0](Type::Int),
                            fn("debug_write_log")[1](Type::String)),

          id[4] | b(Type::String, Call("get_chihayabench")),
          id[3] | callable(fn("open_chihayabench")[any](Type::String)),
          id[16] | b(Type::None, Call("get_lang")));
      return &mp;
    }

    case Type::FrameActionList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::FrameAction),
                                id[2] | b(Type::Callable, Member("size")),
                                id[1] | b(Type::Callable, Member("resize")));
      return &mp;
    }

    case Type::FrameAction: {
      static const auto mp = make_flatmap<Builder>(
          id[1] | callable(fn("start")[any](Type::Int, Type::String)),
          id[3] | callable(fn("start_real")[any](Type::Int, Type::String)),
          id[2] | b(Type::None, Call("end")),
          id[0] | b(Type::Counter, Member("counter")),
          id[4] | b(Type::Int, Member("is_end_action")));
      return &mp;
    }

    case Type::CounterList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::Counter),
                                id[1] | b(Type::Int, Member("size")));
      return &mp;
    }

    case Type::Counter: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | b(Type::Callable, Member("set")),
          id[1] | b(Type::Int, Member("get")),
          id[2] | b(Type::None, Member("reset")),
          id[3] | b(Type::None, Member("start")),
          id[9] | b(Type::None, Member("start_real")),
          id[10] | b(Type::Callable, Member("start_frame")),
          id[11] | b(Type::Callable, Member("start_frame_real")),
          id[12] | b(Type::Callable, Member("start_frame_loop")),
          id[13] | b(Type::Callable, Member("start_frame_loop_real")),
          id[4] | b(Type::None, Member("stop")),
          id[5] | b(Type::None, Member("resume")),
          id[6] | b(Type::Callable, Member("wait")),
          id[8] | b(Type::Callable, Member("wait_key")),
          id[7] | b(Type::Int, Member("check_value")),
          id[14] | b(Type::Int, Member("check_active")));
      return &mp;
    }

    case Type::Syscom: {
      static const auto mp = make_flatmap<Builder>(
          // TODO: 236 -> Syscom_call_ex ?
          id[0] | b(Type::None, Call("menu")),
          id[6] | b(Type::None, Call("menu_enable")),
          id[7] | b(Type::None, Call("menu_disable")),
          id[11] | callable(fn("btn_enable_all")[0](),
                            fn("btn_enable")[1](Type::Int)),
          id[12] | callable(fn("btn_disable_all")[0](),
                            fn("btn_disable")[1](Type::Int)),
          id[133] | b(Type::None, Call("touch_enable")),
          id[134] | b(Type::None, Call("touch_disable")),
          id[5] | b(Type::None, Call("init_flags")),
          // readskip
          id[200] | callable(fn("set_readskip")[any](Type::Int)),
          id[201] | b(Type::Int, Call("get_readskip")),
          id[202] | callable(fn("set_enable_readskip")[any](Type::Int).ret(
                        Type::None)),
          id[203] | b(Type::Int, Call("get_enable_readskip")),
          id[204] | callable(fn("set_exist_readskip")[any](Type::Int).ret(
                        Type::None)),
          id[205] | b(Type::Int, Call("get_exist_readskip")),
          id[206] | b(Type::Int, Call("is_readskip_enable")),
          // autoskip
          id[207] | callable(fn("set_autoskip")[any](Type::Int)),
          id[208] | b(Type::Int, Call("get_autoskip")),
          id[209] | callable(fn("set_enable_autoskip")[any](Type::Int).ret(
                        Type::None)),
          id[210] | b(Type::Int, Call("get_enable_autoskip")),
          id[211] | callable(fn("set_exist_autoskip")[any](Type::Int).ret(
                        Type::None)),
          id[212] | b(Type::Int, Call("get_exist_autoskip")),
          id[213] | b(Type::Int, Call("is_autoskip_enable")),
          // automode
          id[214] | callable(fn("set_automode")[any](Type::Int)),
          id[215] | b(Type::Int, Call("get_automode")),
          id[216] | callable(fn("set_enable_automode")[any](Type::Int).ret(
                        Type::None)),
          id[217] | b(Type::Int, Call("get_enable_automode")),
          id[218] | callable(fn("set_exist_automode")[any](Type::Int).ret(
                        Type::None)),
          id[219] | b(Type::Int, Call("get_exist_automode")),
          id[220] | b(Type::Int, Call("is_automode_enable")),
          // hide mwnd
          id[221] | callable(fn("set_hidemwnd")[any](Type::Int)),
          id[222] | b(Type::Int, Call("get_hidemwnd")),
          id[223] | callable(fn("set_enable_hidemwnd")[any](Type::Int).ret(
                        Type::None)),
          id[224] | b(Type::Int, Call("get_enable_hidemwnd")),
          id[225] | callable(fn("set_exist_hidemwnd")[any](Type::Int).ret(
                        Type::None)),
          id[226] | b(Type::Int, Call("get_exist_hidemwnd")),
          id[227] | b(Type::Int, Call("is_hidemwnd_enable")),
          // extra local switch
          id[300] | callable(fn("set_extraswitch")[any](Type::Int, Type::Int)),
          id[301] |
              callable(fn("get_extraswitch")[any](Type::Int).ret(Type::Int)),
          id[302] |
              callable(fn("set_enable_extraswitch")[any](Type::Int, Type::Int)),
          id[303] | callable(fn("get_enable_extraswitch")[any](Type::Int).ret(
                        Type::Int)),
          id[304] |
              callable(fn("set_exist_extraswitch")[any](Type::Int, Type::Int)),
          id[305] | callable(fn("get_exist_extraswitch")[any](Type::Int).ret(
                        Type::Int)),
          id[306] | callable(fn("is_extraswitch_enable")[any](Type::Int).ret(
                        Type::Int)),
          // local mode
          id[23] | callable(fn("set_localmode")[any](Type::Int)),
          id[57] | b(Type::Int, Call("get_localmode")),
          id[58] | callable(fn("set_enable_localmode")[any](Type::Int).ret(
                       Type::None)),
          id[59] | b(Type::Int, Call("get_enable_localmode")),
          id[62] | callable(fn("set_exist_localmode")[any](Type::Int).ret(
                       Type::None)),
          id[63] | b(Type::Int, Call("get_exist_localmode")),
          id[64] | b(Type::Int, Call("is_localmode_enable")),
          // msgback
          id[192] | b(Type::None, Call("open_msgback")),
          id[193] | b(Type::None, Call("close_msgback")),
          id[194] | callable(fn("set_enable_msgback")[any](Type::Int).ret(
                        Type::None)),
          id[195] | b(Type::Int, Call("get_enable_msgback")),
          id[196] | callable(fn("set_exist_msgback")[any](Type::Int)),
          id[197] | b(Type::Int, Call("get_exist_msgback")),
          id[198] | b(Type::Int, Call("is_msgback_enable")),
          id[329] | b(Type::Int, Call("is_msgback_open")),
          // return to sel
          id[228] | callable(fn("return_to_sel")[any](Type::Int, Type::Int,
                                                      Type::Int)),
          id[230] | callable(fn("set_enable_retsel")[any](Type::Int)),
          id[231] | b(Type::Int, Call("get_enable_retsel")),
          id[232] | callable(fn("set_exist_retsel")[any](Type::Int)),
          id[233] | b(Type::Int, Call("get_exist_retsel")),
          id[234] | b(Type::Int, Call("is_retsel_enable")),
          // return to menu
          id[235] | callable(fn("return_to_menu")[any](
                        Type::Int, Type::Int, Type::Int, kw_arg(0, Type::Int))),
          id[237] | callable(fn("set_enable_retmenu")[any](Type::Int).ret(
                        Type::None)),
          id[238] | b(Type::Int, Call("get_enable_retmenu")),
          id[239] | callable(fn("set_exist_retmenu")[any](Type::Int)),
          id[240] | b(Type::Int, Call("get_exist_retmenu")),
          id[241] | b(Type::Int, Call("is_retmenu_enable")),
          // end game
          id[242] | callable(fn("end_game")[1](Type::Int, Type::Int, Type::Int),
                             fn("end_game")[any](Type::Int)),
          id[244] | callable(fn("set_enable_endgame")[any](Type::Int).ret(
                        Type::None)),
          id[245] | b(Type::Int, Call("get_enable_endgame")),
          id[246] | callable(fn("set_exist_endgame")[any](Type::Int)),
          id[247] | b(Type::Int, Call("get_exist_endgame")),
          id[248] | b(Type::Int, Call("is_endgame_enable")),
          // replay koe
          id[288] | b(Type::None, Call("replay_koe")),
          id[292] | b(Type::Int, Call("check_koe")),
          id[289] | b(Type::Int, Call("get_cur_koe")),
          id[291] | b(Type::Int, Call("get_cur_chr")),
          id[293] | b(Type::None, Call("clear_koe_chr")),
          id[294] | b(Type::String, Call("get_scene_title")),
          id[295] | b(Type::String, Call("get_save_message")),

          id[199] | b(Type::None, Call("get_total_playtime(fixme)")),
          id[229] | callable(fn("set_total_playtime")[any](
                        Type::Int, Type::Int, Type::Int, Type::Int, Type::Int)),

          // save
          id[1] | b(Type::None, Call("open_save")),
          id[251] | callable(fn("set_enable_save")[any](Type::Int)),
          id[252] | b(Type::Int, Call("get_enable_save")),
          id[253] | callable(fn("set_exist_save")[any](Type::Int)),
          id[254] | b(Type::Int, Call("get_exist_save")),
          id[255] | b(Type::Int, Call("is_save_enable")),
          // load
          id[2] | b(Type::None, Call("open_load")),
          id[258] | callable(fn("set_enable_load")[any](Type::Int)),
          id[259] | b(Type::Int, Call("get_enable_load")),
          id[260] | callable(fn("set_exist_load")[any](Type::Int)),
          id[261] | b(Type::Int, Call("get_exist_load")),
          id[262] | b(Type::Int, Call("is_load_enable")),

          // save / load
          id[249] | callable(fn("save")[any](Type::Int, Type::Int, Type::Int)
                                 .ret(Type::Int)),
          id[256] | callable(fn("load")[any](Type::Int, Type::Int, Type::Int,
                                             Type::Int)),
          id[18] | callable(fn("qsave")[any](Type::Int, Type::Int, Type::Int)
                                .ret(Type::Int)),
          id[20] | callable(fn("qload")[any](Type::Int, Type::Int, Type::Int,
                                             Type::Int)),
          id[271] |
              callable(fn("endsave")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[269] |
              callable(fn("endload")[any](Type::Int, Type::Int, Type::Int)),
          // inner save / load
          id[272] | callable(fn("inner_save")[any](Type::Int).ret(Type::Int)),
          id[273] | callable(fn("inner_load")[any](Type::Int, Type::Int,
                                                   Type::Int, Type::Int)
                                 .ret(Type::Int)),
          id[276] |
              callable(fn("clear_inner_save")[any](Type::Int).ret(Type::Int)),
          id[274] |
              callable(fn("check_inner_save")[any](Type::Int).ret(Type::Int)),
          // message back save / load
          id[310] |
              callable(fn("msgbk_load")[any](Type::Int, Type::Int, Type::Int)),
          // save data
          id[68] | b(Type::Int, Call("get_save_count")),
          id[168] | b(Type::Int, Call("get_qsave_count")),
          id[79] | callable(fn("get_new_save_no")[0]().ret(Type::Int),
                            fn("get_new_save_no")[1](Type::Int, Type::Int)
                                .ret(Type::Int)),
          id[170] | callable(fn("get_new_qsave_no")[0]().ret(Type::Int),
                             fn("get_new_qsave_no")[1](Type::Int, Type::Int)
                                 .ret(Type::Int)),
          id[69] | callable(fn("is_save_exist")[any](Type::Int).ret(Type::Int)),
          id[70] | callable(fn("get_save_year")[any](Type::Int).ret(Type::Int)),
          id[71] |
              callable(fn("get_save_month")[any](Type::Int).ret(Type::Int)),
          id[72] | callable(fn("get_save_day")[any](Type::Int).ret(Type::Int)),
          id[73] |
              callable(fn("get_save_weekday")[any](Type::Int).ret(Type::Int)),
          id[74] | callable(fn("get_save_hour")[any](Type::Int).ret(Type::Int)),
          id[75] |
              callable(fn("get_save_minute")[any](Type::Int).ret(Type::Int)),
          id[76] |
              callable(fn("get_save_second")[any](Type::Int).ret(Type::Int)),
          id[77] | callable(fn("get_save_millisecond")[any](Type::Int).ret(
                       Type::Int)),
          id[78] |
              callable(fn("get_save_title")[any](Type::Int).ret(Type::String)),
          id[129] | callable(fn("get_save_message")[any](Type::Int).ret(
                        Type::String)),
          id[324] | callable(fn("get_save_full_message")[any](Type::Int).ret(
                        Type::String)),
          id[131] | callable(fn("get_save_comment")[any](Type::Int).ret(
                        Type::String)),
          id[180] |
              callable(fn("set_save_comment")[any](Type::Int, Type::String)),
          // TODO: 183 -> get_save_value
          // TODO: 182 -> set_save_value
          id[320] | callable(fn("get_save_append_dir")[any](Type::Int).ret(
                        Type::String)),
          id[321] | callable(fn("get_save_append_name")[any](Type::Int).ret(
                        Type::String)),

          id[169] |
              callable(fn("is_qsave_exist")[any](Type::Int).ret(Type::Int)),
          id[171] |
              callable(fn("get_qsave_year")[any](Type::Int).ret(Type::Int)),
          id[172] |
              callable(fn("get_qsave_month")[any](Type::Int).ret(Type::Int)),
          id[173] |
              callable(fn("get_qsave_day")[any](Type::Int).ret(Type::Int)),
          id[174] |
              callable(fn("get_qsave_weekday")[any](Type::Int).ret(Type::Int)),
          id[175] |
              callable(fn("get_qsave_hour")[any](Type::Int).ret(Type::Int)),
          id[176] |
              callable(fn("get_qsave_minute")[any](Type::Int).ret(Type::Int)),
          id[177] |
              callable(fn("get_qsave_second")[any](Type::Int).ret(Type::Int)),
          id[178] | callable(fn("get_qsave_millisecond")[any](Type::Int).ret(
                        Type::Int)),
          id[179] |
              callable(fn("get_qsave_title")[any](Type::Int).ret(Type::String)),
          id[130] | callable(fn("get_qsave_message")[any](Type::Int).ret(
                        Type::String)),
          id[325] | callable(fn("get_qsave_full_message")[any](Type::Int).ret(
                        Type::String)),
          id[132] | callable(fn("get_qsave_comment")[any](Type::Int).ret(
                        Type::String)),
          id[181] |
              callable(fn("set_qsave_comment")[any](Type::Int, Type::String)),
          // TODO: 184 -> get_qsave_value
          // TODO: 185 -> set_qsave_value
          id[322] | callable(fn("get_qsave_append_dir")[any](Type::Int).ret(
                        Type::String)),
          id[323] | callable(fn("get_qsave_append_name")[any](Type::Int).ret(
                        Type::String)),

          id[270] |
              callable(fn("is_endsave_exist")[any](Type::Int).ret(Type::Int)),
          id[67] |
              callable(
                  fn("copy_save")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[22] |
              callable(
                  fn("change_save")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[19] | callable(fn("delete_save")[any](Type::Int).ret(Type::Int)),
          id[128] |
              callable(
                  fn("copy_qsave")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[66] |
              callable(
                  fn("change_qsave")[any](Type::Int, Type::Int).ret(Type::Int)),
          id[65] | callable(fn("delete_qsave")[any](Type::Int).ret(Type::Int)),

          // environment settings
          id[3] | b(Type::None, Call("open_config_menu")),
          id[138] | b(Type::None, Call("open_config_windowmode_menu")),
          id[139] | b(Type::None, Call("open_config_volume_menu")),
          id[137] | b(Type::None, Call("open_config_bgm_fade_menu")),
          id[147] | b(Type::None, Call("open_config_koemode_menu")),
          id[146] | b(Type::None, Call("open_config_charakoe_menu")),
          id[151] | b(Type::None, Call("open_config_jitan_menu")),
          id[135] | b(Type::None, Call("open_config_message_speed_menu")),
          id[136] | b(Type::None, Call("open_config_filter_color_menu")),
          id[140] | b(Type::None, Call("open_config_auto_mode_menu")),
          id[142] | b(Type::None, Call("open_config_font_menu")),
          id[141] | b(Type::None, Call("open_config_system_menu")),
          id[167] | b(Type::None, Call("open_config_movie_menu")),
          // window mode
          id[4] | callable(fn("set_window_mode")[any](Type::Int)),
          id[99] | b(Type::None, Call("set_window_mode_default")),
          id[9] | b(Type::Int, Call("get_window_mode")),
          id[13] |
              callable(fn("set_window_mode_size")[any](Type::Int, Type::Int)),
          id[100] | b(Type::None, Call("set_window_mode_default")),
          id[16] | b(Type::Int, Call("get_window_mode_size")),
          id[309] |
              callable(fn("check_window_mode_size_enable")[any](Type::Int).ret(
                  Type::Int)),
          // volume
          id[39] | callable(fn("set_all_volume")[any](Type::Int)),
          id[21] | callable(fn("set_bgm_volume")[any](Type::Int)),
          id[26] | callable(fn("set_koe_volume")[any](Type::Int)),
          id[29] | callable(fn("set_pcm_volume")[any](Type::Int)),
          id[32] | callable(fn("set_se_volume")[any](Type::Int)),
          id[263] | callable(fn("set_mov_volume")[any](Type::Int)),
          id[277] | callable(fn("set_sound_volume")[any](Type::Int, Type::Int)),
          id[60] | callable(fn("set_all_onoff")[any](Type::Int)),
          id[35] | callable(fn("set_bgm_onoff")[any](Type::Int)),
          id[36] | callable(fn("set_koe_onoff")[any](Type::Int)),
          id[37] | callable(fn("set_pcm_onoff")[any](Type::Int)),
          id[38] | callable(fn("set_se_onoff")[any](Type::Int)),
          id[266] | callable(fn("set_mov_onoff")[any](Type::Int)),
          id[280] | callable(fn("set_sound_onoff")[any](Type::Int, Type::Int)),
          id[40] | b(Type::None, Call("set_all_volume_default")),
          id[24] | b(Type::None, Call("set_bgm_volume_default")),
          id[27] | b(Type::None, Call("set_koe_volume_default")),
          id[30] | b(Type::None, Call("set_pcm_volume_default")),
          id[33] | b(Type::None, Call("set_se_volume_default")),
          id[264] | b(Type::None, Call("set_mov_volume_default")),
          id[278] | callable(fn("set_sound_volume_default")[any](Type::Int).ret(
                        Type::None)),
          id[101] | b(Type::None, Call("set_all_onoff_default")),
          id[102] | b(Type::None, Call("set_bgm_onoff_default")),
          id[103] | b(Type::None, Call("set_koe_onoff_default")),
          id[104] | b(Type::None, Call("set_pcm_onoff_default")),
          id[105] | b(Type::None, Call("set_se_onoff_default")),
          id[267] | b(Type::None, Call("set_mov_onoff_default")),
          id[281] | callable(fn("set_sound_onoff_default")[any](Type::Int).ret(
                        Type::None)),
          id[41] | b(Type::Int, Call("get_all_volume")),
          id[25] | b(Type::Int, Call("get_bgm_volume")),
          id[28] | b(Type::Int, Call("get_koe_volume")),
          id[31] | b(Type::Int, Call("get_pcm_volume")),
          id[34] | b(Type::Int, Call("get_se_volume")),
          id[265] | b(Type::Int, Call("get_mov_volume")),
          id[279] |
              callable(fn("get_sound_volume")[any](Type::Int).ret(Type::Int)),
          id[61] | b(Type::Int, Call("get_all_onoff")),
          id[42] | b(Type::Int, Call("get_bgm_onoff")),
          id[43] | b(Type::Int, Call("get_koe_onoff")),
          id[44] | b(Type::Int, Call("get_pcm_onoff")),
          id[45] | b(Type::Int, Call("get_se_onoff")),
          id[268] | b(Type::Int, Call("get_mov_onoff")),
          id[282] |
              callable(fn("get_sound_onoff")[any](Type::Int).ret(Type::Int)),
          // bgm fade
          id[94] | callable(fn("set_bgmfade_volume")[any](Type::Int).ret(
                       Type::None)),
          id[97] | callable(fn("set_bgmfade_onoff")[any](Type::Int)),
          id[95] | b(Type::None, Call("set_bgmfade_volume_default")),
          id[106] | b(Type::None, Call("set_bgmfade_onoff_default")),
          id[96] | b(Type::Int, Call("get_bgmfade_volume")),
          id[98] | b(Type::Int, Call("get_bgmfade_onoff")),
          // koemode
          id[148] | callable(fn("set_koemode")[any](Type::Int)),
          id[149] | b(Type::None, Call("set_koemode_default")),
          id[150] | b(Type::Int, Call("get_koemode")),
          // character koe
          id[143] |
              callable(fn("set_charakoe_onoff")[any](Type::Int, Type::Int)),
          id[144] |
              callable(fn("set_charakoe_onoff_default")[any](Type::Int).ret(
                  Type::None)),
          id[145] |
              callable(fn("get_charakoe_onoff")[any](Type::Int).ret(Type::Int)),
          id[186] |
              callable(fn("set_charakoe_volume")[any](Type::Int, Type::Int)),
          id[187] |
              callable(fn("set_charakoe_volume_default")[any](Type::Int).ret(
                  Type::None)),
          id[188] | callable(fn("get_charakoe_volume")[any](Type::Int).ret(
                        Type::Int)),
          // jitan
          id[153] | callable(fn("set_jitan_normal_onoff")[any](Type::Int).ret(
                        Type::None)),
          id[154] | b(Type::None, Call("set_jitan_normal_onoff_default")),
          id[155] | b(Type::Int, Call("get_jitan_normal_onoff")),
          id[156] |
              callable(fn("set_jitan_auto_mode_onoff")[any](Type::Int).ret(
                  Type::None)),
          id[157] | b(Type::None, Call("set_jitan_auto_mode_onoff_default")),
          id[158] | b(Type::Int, Call("get_jitan_auto_mode_onoff")),
          id[159] |
              callable(fn("set_jitan_koe_replay_onoff")[any](Type::Int).ret(
                  Type::None)),
          id[160] | b(Type::None, Call("set_jitan_koe_replay_onoff_default")),
          id[161] | b(Type::Int, Call("get_jitan_koe_replay_onoff")),
          id[152] | callable(fn("set_jitan_speed")[any](Type::Int)),
          id[162] | b(Type::None, Call("set_jitan_speed_default")),
          id[163] | b(Type::Int, Call("get_jitan_speed")),
          // message speed
          id[46] | callable(fn("set_message_speed")[any](Type::Int)),
          id[47] | b(Type::None, Call("set_message_speed_default")),
          id[48] | b(Type::Int, Call("get_message_speed")),
          id[49] | callable(fn("set_message_nowait")[any](Type::Int).ret(
                       Type::None)),
          id[107] | b(Type::None, Call("set_message_nowait_default")),
          id[50] | b(Type::Int, Call("get_message_nowait")),
          // auto mode
          id[51] | callable(fn("set_auto_mode_moji_wait")[any](Type::Int).ret(
                       Type::None)),
          id[52] | b(Type::None, Call("set_auto_mode_moji_wait_default")),
          id[53] | b(Type::Int, Call("get_auto_mode_moji_wait")),
          id[54] | callable(fn("set_auto_mode_min_wait")[any](Type::Int).ret(
                       Type::None)),
          id[55] | b(Type::None, Call("set_auto_mode_min_wait_default")),
          id[56] | b(Type::Int, Call("get_auto_mode_min_wait")),
          // auto hide mouse cursor
          id[311] |
              callable(fn("set_mouse_cursor_hide_onoff")[any](Type::Int).ret(
                  Type::None)),
          id[312] | b(Type::None, Call("set_mouse_cursor_hide_onoff_default")),
          id[313] | b(Type::Int, Call("get_mouse_cursor_hide_onoff")),
          id[317] |
              callable(fn("set_mouse_cursor_hide_time")[any](Type::Int).ret(
                  Type::None)),
          id[318] | b(Type::None, Call("set_mouse_cursor_hide_time_default")),
          id[319] | b(Type::Int, Call("get_mouse_cursor_hide_time")),
          // window background color
          id[82] | callable(fn("set_filter_color_r")[any](Type::Int).ret(
                       Type::None)),
          id[85] | callable(fn("set_filter_color_g")[any](Type::Int).ret(
                       Type::None)),
          id[86] | callable(fn("set_filter_color_b")[any](Type::Int).ret(
                       Type::None)),
          id[87] | callable(fn("set_filter_color_a")[any](Type::Int).ret(
                       Type::None)),
          id[83] | b(Type::None, Call("set_filter_color_r_default")),
          id[88] | b(Type::None, Call("set_filter_color_g_default")),
          id[89] | b(Type::None, Call("set_filter_color_b_default")),
          id[90] | b(Type::None, Call("set_filter_color_a_default")),
          id[84] | b(Type::Int, Call("get_filter_color_r")),
          id[91] | b(Type::Int, Call("get_filter_color_g")),
          id[92] | b(Type::Int, Call("get_filter_color_b")),
          id[93] | b(Type::Int, Call("get_filter_color_a")),
          // display object
          id[189] |
              callable(fn("set_obj_disp_onoff")[any](Type::Int, Type::Int)),
          id[190] | callable(fn("set_obj_disp_onoff_default")[any](Type::Int)),
          id[191] |
              callable(fn("get_obj_disp_onoff")[any](Type::Int).ret(Type::Int)),
          // global extra switch
          id[14] | callable(fn("set_global_extraswitch_onoff")[any](Type::Int,
                                                                    Type::Int)),
          id[15] | callable(fn("set_global_extraswitch_onoff_default")[any](
                       Type::Int)),
          id[17] |
              callable(fn("get_global_extraswitch_onoff")[any](Type::Int).ret(
                  Type::Int)),
          // global extra mode
          id[164] |
              callable(fn("set_global_extramode")[any](Type::Int, Type::Int)),
          id[165] |
              callable(fn("set_global_extramode_default")[any](Type::Int)),
          id[166] |
              callable(fn("get_global_extramode_default")[any](Type::Int).ret(
                  Type::Int)),

          // system settings
          id[80] | callable(fn("set_saveload_alert_onoff")[any](Type::Int)),
          id[108] | b(Type::None, Call("set_saveload_alert_onoff_default")),
          id[10] | b(Type::Int, Call("get_saveload_alert_onoff")),
          id[110] | callable(fn("set_sleep_onoff")[any](Type::Int)),
          id[111] | b(Type::None, Call("set_sleep_onoff_default")),
          id[112] | b(Type::Int, Call("get_sleep_onoff")),
          id[113] | callable(fn("set_no_wipe_anime_onoff")[any](Type::Int)),
          id[114] | b(Type::None, Call("set_no_wipe_anime_onoff_default")),
          id[115] | b(Type::Int, Call("get_no_wipe_anime_onoff")),
          id[116] | callable(fn("set_skip_wipe_anime_onoff")[any](Type::Int)),
          id[117] | b(Type::None, Call("set_skip_wipe_anime_onoff_default")),
          id[118] | b(Type::Int, Call("get_skip_wipe_anime_onoff")),
          id[8] | callable(fn("set_no_mwnd_anime_onoff")[any](Type::Int)),
          id[109] | b(Type::None, Call("set_no_mwnd_anime_onoff_default")),
          id[81] | b(Type::Int, Call("get_no_mwnd_anime_onoff")),
          id[119] |
              callable(fn("set_wheel_next_message_onoff")[any](Type::Int)),
          id[120] | b(Type::None, Call("set_wheel_next_message_onoff_default")),
          id[121] | b(Type::Int, Call("get_wheel_next_message_onoff")),
          id[122] | callable(fn("set_koe_dont_stop_onoff")[any](Type::Int)),
          id[123] | b(Type::None, Call("set_koe_dont_stop_onoff_default")),
          id[124] | b(Type::Int, Call("get_koe_dont_stop_onoff")),
          id[125] |
              callable(fn("set_skip_unread_message_onoff")[any](Type::Int)),
          id[126] |
              b(Type::None, Call("set_skip_unread_message_onoff_default")),
          id[127] | b(Type::Int, Call("get_skip_unread_message_onoff")),
          id[250] | callable(fn("set_play_silent_sound_onoff")[any](Type::Int)),
          id[247] | b(Type::None, Call("set_play_silent_sound_onoff_default")),
          id[243] | b(Type::Int, Call("get_play_silent_sound_onoff")),

          // font
          id[283] | callable(fn("set_font_name")[any](Type::String)),
          id[326] | b(Type::None, Call("set_font_name_default")),
          id[284] | b(Type::String, Call("get_font_name")),
          id[285] |
              callable(fn("is_font_exist")[any](Type::String).ret(Type::Int)),
          id[296] | callable(fn("set_font_bold")[any](Type::Int)),
          id[298] | b(Type::None, Call("set_font_bold_default")),
          id[307] | b(Type::Int, Call("get_font_bold")),
          id[297] | callable(fn("set_font_decoration")[any](Type::Int)),
          id[299] | b(Type::None, Call("set_font_decoration_default")),
          id[308] | b(Type::Int, Call("get_font_decoration")),

          // capture
          id[286] |
              callable(fn("create_capture_buffer")[any](Type::Int, Type::Int)),
          id[287] | b(Type::None, Call("destroy_capture_buffer")),
          id[316] | callable(fn("capture_to_capture_buffer")[any](Type::Int,
                                                                  Type::Int)),
          id[314] | callable(fn("save_capture_buffer_to_file")[any](
                                 Type::String, Type::String,
                                 kw_arg(0, Type::Int), kw_arg(1, Type::String),
                                 kw_arg(2, Type::Other), kw_arg(3, Type::Int),
                                 kw_arg(4, Type::Int), kw_arg(5, Type::Other),
                                 kw_arg(6, Type::Int), kw_arg(7, Type::Int))
                                 .ret(Type::Int)),
          id[315] | callable(fn("load_flag_from_capture_file")[any](
                                 Type::String, Type::String,
                                 kw_arg(0, Type::Int), kw_arg(1, Type::String),
                                 kw_arg(2, Type::Other), kw_arg(3, Type::Int),
                                 kw_arg(4, Type::Int), kw_arg(5, Type::Other),
                                 kw_arg(6, Type::Int), kw_arg(7, Type::Int))
                                 .ret(Type::Int)),
          id[290] | callable(fn("capture_to_png")[any](Type::Int, Type::Int,
                                                       Type::String)),
          id[327] | b(Type::None, Call("twitter")),
          id[328] |
              callable(fn("set_ret_scene_once")[0](Type::String),
                       fn("set_ret_scene_once")[any](Type::String, Type::Int)),

          id[330] |
              callable(fn("get_sys_extra_int")[any](Type::Int).ret(Type::Int)),
          id[331] | callable(fn("get_sys_extra_str")[any](Type::Int).ret(
                        Type::String)));
      return &mp;
    }

    case Type::Excall: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | b_index_array(Type::Excall),
          id[4] | b(Type::None, Member("alloc")),
          id[5] | b(Type::None, Member("free")),
          id[12] | b(Type::Int, Member("is_excall")),
          id[8] | b(Type::Int, Member("check_alloc")),
          id[7] | b(Type::IntList, Member("F")),
          id[6] | b(Type::CounterList, Member("counter")),
          id[9] | b(Type::FrameAction, Member("frame_action")),
          id[10] | b(Type::FrameActionList, Member("frame_action_ch")),
          id[0] | b(Type::StageList, Member("stage")),
          id[2] | b(Type::Stage, Member("back")),
          id[1] | b(Type::Stage, Member("front")),
          id[3] | b(Type::Stage, Member("next")),
          id[13] | b(Type::ScriptExcall, Member("script")));
      return &mp;
    }

    case Type::StageList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::Stage));
      return &mp;
    }
    case Type::Stage: {
      static const auto mp = make_flatmap<Builder>(
          id[2] | b(Type::ObjList, Member("object")),
          id[3] | b(Type::MwndList, Member("mwnd")),
          id[6] | b(Type::GroupList, Member("objgroup")),
          id[5] | b(Type::BtnselItemList, Member("btnsel")),
          id[8] | b(Type::WorldList, Member("world")),
          id[4] | b(Type::EffectList, Member("effect")),
          id[7] | b(Type::QuakeList, Member("quake")),
          id[0] | b(Type::Callable, Member("create_obj")),
          id[1] | b(Type::Callable, Member("create_mwnd")));
      return &mp;
    }

    case Type::ObjList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::Object),
                                id[4] | b(Type::Callable, Member("resize")),
                                id[3] | b(Type::Callable, Member("size")));
      return &mp;
    }
    case Type::Object: {
      auto obj_createmov = [](std::string_view mem) -> Builder {
        return callable(
            fn(mem)[2](Type::String, Type::Int, Type::Int, Type::Int,
                       kw_arg(0, Type::Int), kw_arg(1, Type::Int),
                       kw_arg(2, Type::Int)),
            fn(mem)[1](Type::String, Type::Int, kw_arg(0, Type::Int),
                       kw_arg(1, Type::Int), kw_arg(2, Type::Int)),
            fn(mem)[any](Type::String, kw_arg(0, Type::Int),
                         kw_arg(1, Type::Int), kw_arg(2, Type::Int)));
      };

      static const auto mp = make_flatmap<Builder>(
          id[56] | obj_getset("wipe_copy", Type::Int),
          id[92] | obj_getset("wipe_erase", Type::Int),
          id[139] | obj_getset("click_disable", Type::Int),
          id[0] | obj_getset("disp", Type::Int),
          id[1] | obj_getset("patno", Type::Int),
          id[44] | obj_getset("world", Type::Int),
          id[55] | obj_getset("order", Type::Int),
          id[2] | obj_getset("layer", Type::Int),
          id[3] | obj_getset("x", Type::Int),
          id[4] | obj_getset("y", Type::Int),
          id[5] | obj_getset("z", Type::Int),
          id[54] | b(Type::IntList, Member("x_rep")),
          id[63] | b(Type::IntList, Member("y_rep")),
          id[110] | b(Type::IntList, Member("z_rep")),
          id[48] | callable(fn("set_pos")[0](Type::Int, Type::Int),
                            fn("set_pos")[1](Type::Int, Type::Int, Type::Int)),
          id[6] | obj_getset("center_x", Type::Int),
          id[7] | obj_getset("center_y", Type::Int),
          id[8] | obj_getset("center_z", Type::Int),
          id[158] |
              callable(fn("set_center")[0](Type::Int, Type::Int),
                       fn("set_center")[1](Type::Int, Type::Int, Type::Int)),
          id[9] | obj_getset("center_rep_x", Type::Int),
          id[10] | obj_getset("center_rep_y", Type::Int),
          id[11] | obj_getset("center_rep_z", Type::Int),
          id[159] | callable(fn("set_center_rep")[0](Type::Int, Type::Int),
                             fn("set_center_rep")[1](Type::Int, Type::Int,
                                                     Type::Int)),
          id[12] | obj_getset("scale_x", Type::Int),
          id[13] | obj_getset("scale_y", Type::Int),
          id[14] | obj_getset("scale_z", Type::Int),
          id[49] |
              callable(fn("set_scale")[0](Type::Int, Type::Int),
                       fn("set_scale")[1](Type::Int, Type::Int, Type::Int)),
          id[15] | obj_getset("rotate_x", Type::Int),
          id[16] | obj_getset("rotate_y", Type::Int),
          id[17] | obj_getset("rotate_z", Type::Int),
          id[50] |
              callable(fn("set_rotate")[0](Type::Int, Type::Int),
                       fn("set_rotate")[1](Type::Int, Type::Int, Type::Int)),
          id[18] | obj_getset("clip_use", Type::Int),
          id[19] | obj_getset("clip_left", Type::Int),
          id[20] | obj_getset("clip_top", Type::Int),
          id[21] | obj_getset("clip_right", Type::Int),
          id[22] | obj_getset("clip_bottom", Type::Int),
          id[160] | callable(fn("set_clip")[any](
                        Type::Int, Type::Int, Type::Int, Type::Int, Type::Int)),
          id[149] | obj_getset("src_clip_use", Type::Int),
          id[150] | obj_getset("src_clip_left", Type::Int),
          id[151] | obj_getset("src_clip_top", Type::Int),
          id[152] | obj_getset("src_clip_right", Type::Int),
          id[153] | obj_getset("src_clip_bottom", Type::Int),
          id[161] | callable(fn("set_src_clip")[any](
                        Type::Int, Type::Int, Type::Int, Type::Int, Type::Int)),
          id[27] | obj_getset("tr", Type::Int),
          id[141] | b(Type::IntList, Member("tr_rep")),
          id[28] | obj_getset("mono", Type::Int),
          id[29] | obj_getset("reverse", Type::Int),
          id[30] | obj_getset("bright", Type::Int),
          id[31] | obj_getset("dark", Type::Int),
          id[32] | obj_getset("color_r", Type::Int),
          id[33] | obj_getset("color_g", Type::Int),
          id[34] | obj_getset("color_b", Type::Int),
          id[23] | obj_getset("color_rate", Type::Int),
          id[57] | obj_getset("color_add_r", Type::Int),
          id[58] | obj_getset("color_add_g", Type::Int),
          id[59] | obj_getset("color_add_b", Type::Int),
          id[145] | obj_getset("mask_no", Type::Int),
          id[109] | obj_getset("tonecurve_no", Type::Int),
          id[146] | obj_getset("culling", Type::Int),
          id[147] | obj_getset("alpha_test", Type::Int),
          id[148] | obj_getset("alpha_blend", Type::Int),
          id[46] | obj_getset("blend", Type::Int),
          id[168] | obj_getset("light_no", Type::Int),
          id[144] | obj_getset("fog_use", Type::Int),
          id[90] | b(Type::IntEvent, Member("patno_eve")),
          id[51] | b(Type::IntEvent, Member("x_eve")),
          id[64] | b(Type::IntEvent, Member("y_eve")),
          id[65] | b(Type::IntEvent, Member("z_eve")),
          id[112] | b(Type::IntEventList, Member("x_rep_eve")),
          id[113] | b(Type::IntEventList, Member("y_rep_eve")),
          id[114] | b(Type::IntEventList, Member("z_rep_eve")),
          id[77] | b(Type::IntEvent, Member("center_x_eve")),
          id[78] | b(Type::IntEvent, Member("center_y_eve")),
          id[79] | b(Type::IntEvent, Member("center_z_eve")),
          id[80] | b(Type::IntEvent, Member("center_rep_x_eve")),
          id[81] | b(Type::IntEvent, Member("center_rep_y_eve")),
          id[82] | b(Type::IntEvent, Member("center_rep_z_eve")),
          id[67] | b(Type::IntEvent, Member("scale_x_eve")),
          id[68] | b(Type::IntEvent, Member("scale_y_eve")),
          id[66] | b(Type::IntEvent, Member("scale_z_eve")),
          id[69] | b(Type::IntEvent, Member("rotate_x_eve")),
          id[70] | b(Type::IntEvent, Member("rotate_y_eve")),
          id[71] | b(Type::IntEvent, Member("rotate_z_eve")),
          id[105] | b(Type::IntEvent, Member("clip_left_eve")),
          id[106] | b(Type::IntEvent, Member("clip_top_eve")),
          id[107] | b(Type::IntEvent, Member("clip_right_eve")),
          id[108] | b(Type::IntEvent, Member("clip_bottom_eve")),
          id[154] | b(Type::IntEvent, Member("src_clip_left_eve")),
          id[155] | b(Type::IntEvent, Member("src_clip_top_eve")),
          id[156] | b(Type::IntEvent, Member("src_clip_right_eve")),
          id[157] | b(Type::IntEvent, Member("src_clip_bottom_eve")),
          id[72] | b(Type::IntEvent, Member("tr_eve")),
          id[140] | b(Type::IntEvent, Member("tr_rep_eve")),
          id[73] | b(Type::IntEvent, Member("mono_eve")),
          id[74] | b(Type::IntEvent, Member("reverse_eve")),
          id[75] | b(Type::IntEvent, Member("bright_eve")),
          id[76] | b(Type::IntEvent, Member("dark_eve")),
          id[87] | b(Type::IntEvent, Member("color_r_eve")),
          id[88] | b(Type::IntEvent, Member("color_g_eve")),
          id[89] | b(Type::IntEvent, Member("color_b_eve")),
          id[83] | b(Type::IntEvent, Member("color_rate_eve")),
          id[84] | b(Type::IntEvent, Member("color_add_r_eve")),
          id[85] | b(Type::IntEvent, Member("color_add_g_eve")),
          id[86] | b(Type::IntEvent, Member("color_add_b_eve")),
          id[91] | Builder([](Builder::Ctx& ctx) {
            const int peek = AsInt(ctx.elmcode[1]);
            ctx.chain.nodes.emplace_back(Type::Invalid, Member("alleve"));
            switch (peek) {
              case 0:  // ALLEVENT_END
                ctx.chain.nodes.emplace_back(Type::None, Call("end"));
                return;
              case 1:
                ctx.chain.nodes.emplace_back(Type::None, Call("wait"));
                return;
              case 2:
                ctx.chain.nodes.emplace_back(Type::Int, Call("check"));
                return;
              default:
                ctx.chain.nodes.emplace_back(Type::Invalid, Member("???"));
                return;
            }
          }),
          id[169] | b(Type::Int, Member("pat_cnt")),

          id[24] | obj_getter("get_size_x"), id[25] | obj_getter("get_size_y"),
          id[47] | obj_getter("get_size_z"),
          id[100] | obj_colgetter("get_pixel_color_a"),
          id[101] | obj_colgetter("get_pixel_color_r"),
          id[102] | obj_colgetter("get_pixel_color_g"),
          id[103] | obj_colgetter("get_pixel_color_b"),

          id[62] | b(Type::String, Call("get_file_path")),
          id[111] | b(Type::IntList, Member("F")),

          id[93] | b(Type::ObjList, Member("child")),
          id[35] | b(Type::None, Call("init")),
          id[36] | b(Type::None, Call("free")),
          id[37] | b(Type::None, Call("init_param")),
          id[38] | callable(fn("create")[3](Type::String, Type::Int, Type::Int,
                                            Type::Int, Type::Int),
                            fn("create")[2](Type::String, Type::Int, Type::Int,
                                            Type::Int),
                            fn("create")[1](Type::String, Type::Int),
                            fn("crate")[any](Type::String)),
          id[40] |
              callable(fn("create_rect")[2](Type::Int, Type::Int, Type::Int,
                                            Type::Int, Type::Int, Type::Int,
                                            Type::Int, Type::Int, Type::Int,
                                            Type::Int, Type::Int),
                       fn("create_rect")[1](Type::Int, Type::Int, Type::Int,
                                            Type::Int, Type::Int, Type::Int,
                                            Type::Int, Type::Int, Type::Int),
                       fn("create_rect")[any](Type::Int, Type::Int, Type::Int,
                                              Type::Int, Type::Int, Type::Int,
                                              Type::Int, Type::Int)),
          id[39] | callable(fn("create_string")[2](Type::String, Type::Int,
                                                   Type::Int, Type::Int),
                            fn("create_string")[1](Type::String, Type::Int),
                            fn("create_string")[any](Type::String)),
          id[132] | callable(fn("create_number")[2](Type::String, Type::Int,
                                                    Type::Int, Type::Int),
                             fn("create_number")[1](Type::String, Type::Int),
                             fn("create_number")[any](Type::String)),
          id[129] | callable(fn("create_weather")[2](Type::String, Type::Int,
                                                     Type::Int, Type::Int),
                             fn("create_weather")[1](Type::String, Type::Int),
                             fn("create_weather")[any](Type::String)),
          id[43] | callable(fn("create_mesh")[2](Type::String, Type::Int,
                                                 Type::Int, Type::Int),
                            fn("create_mesh")[1](Type::String, Type::Int),
                            fn("create_mesh")[any](Type::String)),
          id[45] | callable(fn("create_billboard")[2](Type::String, Type::Int,
                                                      Type::Int, Type::Int),
                            fn("create_billboard")[1](Type::String, Type::Int),
                            fn("create_billboard")[any](Type::String)),
          id[116] | callable(fn("create_save_thumb")[2](Type::Int, Type::Int,
                                                        Type::Int, Type::Int),
                             fn("create_save_thumb")[1](Type::Int, Type::Int),
                             fn("create_save_thumb")[any](Type::Int)),
          id[170] |
              callable(fn("create_capture_thumb")[2](Type::Int, Type::Int,
                                                     Type::Int, Type::Int),
                       fn("create_capture_thumb")[1](Type::Int, Type::Int),
                       fn("create_capture_thumb")[any](Type::Int)),

          id[165] |
              callable(fn("create_capture")[2](Type::Int, Type::Int, Type::Int),
                       fn("create_capture")[1](Type::Int),
                       fn("create_capture")[any]()),
          id[120] | obj_createmov("create_movie"),
          id[121] | obj_createmov("create_movie_loop"),
          id[122] | obj_createmov("create_movie_wait"),
          id[143] | obj_createmov("create_movie_waitkey"),
          id[177] | callable(fn("create_emote")[2](
                                 Type::Int, Type::Int, Type::String, Type::Int,
                                 Type::Int, Type::Int, kw_arg(0, Type::Int),
                                 kw_arg(1, Type::Int)),
                             fn("create_emote")[1](
                                 Type::Int, Type::Int, Type::String, Type::Int,
                                 kw_arg(0, Type::Int), kw_arg(1, Type::Int)),
                             fn("create_emote")[any](
                                 Type::Int, Type::Int, Type::String,
                                 kw_arg(0, Type::Int), kw_arg(1, Type::Int))),

          id[41] | callable(fn("copy_from")[any](Type::Object)),
          id[53] | callable(fn("change_file")[any](Type::String)),
          id[174] | b(Type::Int, Call("exist_type")),

          // String
          id[99] | callable(fn("set_string")[any](Type::String)),
          id[135] | b(Type::String, Call("get_string")),
          id[104] | callable(fn("set_string_param")[1](Type::Int, Type::Int,
                                                       Type::Int, Type::Int,
                                                       Type::Int, Type::Int,
                                                       Type::Int, Type::Int),
                             fn("set_string_param")[0](Type::Int, Type::Int,
                                                       Type::Int, Type::Int,
                                                       Type::Int, Type::Int,
                                                       Type::Int),
                             fn("set_string_param")[any](Type::Int, Type::Int,
                                                         Type::Int, Type::Int,
                                                         Type::Int)),
          id[133] | callable(fn("set_number")[any](Type::Int)),
          id[136] | b(Type::Int, Call("set_number")),
          id[134] | callable(fn("set_number_param")[any](Type::Int, Type::Int,
                                                         Type::Int, Type::Int,
                                                         Type::Int, Type::Int)),

          // environment
          id[130] | callable(fn("set_weather_param_type_a")[any](
                        kw_arg(0, Type::Int), kw_arg(1, Type::Int),
                        kw_arg(2, Type::Int), kw_arg(3, Type::Int),
                        kw_arg(4, Type::Int), kw_arg(5, Type::Int),
                        kw_arg(6, Type::Int), kw_arg(7, Type::Int),
                        kw_arg(8, Type::Int), kw_arg(9, Type::Int),
                        kw_arg(10, Type::Int), kw_arg(11, Type::Int),
                        kw_arg(12, Type::Int), kw_arg(13, Type::Int),
                        kw_arg(14, Type::Int))),
          id[131] | callable(fn("set_weather_param_type_a")[any](
                        kw_arg(0, Type::Int), kw_arg(1, Type::Int),
                        kw_arg(2, Type::Int), kw_arg(3, Type::Int),
                        kw_arg(4, Type::Int), kw_arg(5, Type::Int),
                        kw_arg(6, Type::Int), kw_arg(7, Type::Int),
                        kw_arg(8, Type::Int), kw_arg(9, Type::Int),
                        kw_arg(10, Type::Int), kw_arg(11, Type::Int),
                        kw_arg(12, Type::Int), kw_arg(13, Type::Int),
                        kw_arg(14, Type::Int), kw_arg(15, Type::Int),
                        kw_arg(16, Type::Int), kw_arg(17, Type::Int),
                        kw_arg(18, Type::Int), kw_arg(19, Type::Int))),

          // movie
          id[125] | b(Type::None, Call("pause_movie")),
          id[126] | b(Type::None, Call("resume_movie")),
          id[137] | callable(fn("seek_movie")[any](Type::Int)),
          id[138] | b(Type::Int, Call("get_movie_seek_time")),
          id[127] | b(Type::Int, Call("check_movie")),
          id[128] | b(Type::None, Call("wait_movie")),
          id[142] | b(Type::None, Call("wait_movie_key")),
          id[171] | b(Type::None, Call("end_movie_loop")),
          id[172] | callable(fn("set_movie_auto_free")[any](Type::Int)),

          // frame action
          id[52] | b(Type::FrameAction, Member("frame_action")),
          id[115] | b(Type::FrameActionList, Member("frame_action_ch")),

          // button
          id[61] | b(Type::None, Call("clear_button")),
          id[42] | callable(fn("set_button")[2](Type::Int, Type::Int, Type::Int,
                                                Type::Int),
                            fn("set_button")[1](Type::Int, Type::Int),
                            fn("set_button")[0](Type::Int),
                            fn("set_button")[any]()),
          id[164] | callable(fn("set_button_group")[0](Type::Int),
                             fn("set_button_group")[1](Type::Other)),
          id[98] | callable(fn("set_button_pushkeep")[any](Type::Int)),
          id[119] | b(Type::Int, Call("get_button_pushkeep")),
          id[175] | callable(fn("set_button_alpha_test")[any](Type::Int)),
          id[176] | b(Type::Int, Call("get_button_alpha_test")),

          id[95] | b(Type::None, Call("set_button_state_normal")),
          id[96] | b(Type::None, Call("set_button_state_select")),
          id[97] | b(Type::None, Call("set_button_state_disable")),
          id[118] | b(Type::Int, Call("get_button_state")),
          id[123] | b(Type::Int, Call("get_button_hit_state")),
          id[124] | b(Type::Int, Call("get_button_real_state")),
          id[26] | callable(fn("set_button_call")[any](Type::String)),
          id[60] | b(Type::None, Call("clear_button_call"))

          // GAN

      );
      return &mp;
    }

    case Type::MaskList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::Mask),
                                id[1] | b(Type::Int, Member("size")));
      return &mp;
    }

    case Type::Mask: {
      static const auto mp = make_flatmap<Builder>(
          id[1] | b(Type::None, Member("init")),
          id[0] | b(Type::None, Member("create")),
          id[4] | b(Type::Int, Member("x")), id[5] | b(Type::Int, Member("y")),
          id[2] | b(Type::IntEvent, Member("x_eve")),
          id[3] | b(Type::IntEvent, Member("y_eve")));
      return &mp;
    }

    case Type::Movie: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | callable(fn("play")[any](Type::String),
                           fn("play")[any](Type::String, Type::Int, Type::Int,
                                           Type::Int, Type::Int)),
          id[2] |
              callable(fn("play_wait")[any](Type::String),
                       fn("play_wait")[any](Type::String, Type::Int, Type::Int,
                                            Type::Int, Type::Int)),
          id[3] | callable(
                      fn("play_waitkey")[any](Type::String),
                      fn("play_waitkey")[any](Type::String, Type::Int,
                                              Type::Int, Type::Int, Type::Int)),
          id[1] | b(Type::None, Call("stop")));
      return &mp;
    }

    case Type::BgmTable: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | b(Type::Int, Member("cnt")),
          id[2] | callable(fn("set_listen")[any](Type::String, Type::Int)),
          id[4] | callable(fn("set_listen_all")[any](Type::Int)),
          id[1] | callable(fn("get_listen")[any](Type::String).ret(Type::Int)));
      return &mp;
    }

    case Type::Bgm: {
      static const auto mp = make_flatmap<Builder>(
          // TODO: handle overload with default argument
          id[0] | callable(fn("play")[any](
                      Type::String, Type::Int, Type::Int,
                      kw_arg(0, Type::String), kw_arg(1, Type::Int),
                      kw_arg(2, Type::Int), kw_arg(3, Type::Int),
                      kw_arg(4, Type::Int), kw_arg(5, Type::Int))),
          id[1] | callable(fn("play_oneshot")[0](Type::String),
                           fn("play_oneshot")[1](Type::String, Type::Int),
                           fn("play_oneshot")[2](Type::String, Type::Int,
                                                 Type::Int)),
          id[2] |
              callable(fn("play_wait")[0](Type::String),
                       fn("play_wait")[1](Type::String, Type::Int),
                       fn("play_wait")[2](Type::String, Type::Int, Type::Int)),
          id[16] |
              callable(
                  fn("ready")[0](Type::String, kw_arg(0, Type::String),
                                 kw_arg(1, Type::Int), kw_arg(3, Type::Int),
                                 kw_arg(5, Type::Int)),
                  fn("ready")[2](Type::String, Type::Int,
                                 kw_arg(0, Type::String), kw_arg(1, Type::Int),
                                 kw_arg(3, Type::Int), kw_arg(5, Type::Int))),
          id[4] | callable(fn("stop")[0](), fn("stop")[1](Type::Int)),
          id[10] | callable(fn("pause")[0](), fn("pause")[1](Type::Int)),
          id[11] | callable(fn("resume")[0](kw_arg(0, Type::Int)),
                            fn("resume")[1](Type::Int, kw_arg(0, Type::Int))),
          id[12] |
              callable(fn("resume_wait")[0](), fn("resume_wait")[1](Type::Int)),
          id[3] | b(Type::None, Call("wait")),
          id[14] | b(Type::None, Call("wait_key")),
          id[5] | b(Type::None, Call("wait_fade")),
          id[15] | b(Type::None, Call("wait_fade_key")),
          id[18] | b(Type::Int, Call("check")),
          id[6] | callable(fn("set_volume")[0](Type::Int),
                           fn("set_volume")[1](Type::Int, Type::Int)),
          id[7] | callable(fn("set_volume_max")[0](Type::Int),
                           fn("set_volume_max")[1](Type::Int, Type::Int)),
          id[8] | callable(fn("set_volume_min")[0](Type::Int),
                           fn("set_volume_min")[1](Type::Int, Type::Int)),
          id[9] | b(Type::Int, Call("get_volume")),
          id[19] | b(Type::String, Call("get_regist_name")),
          id[13] | b(Type::Int, Call("get_play_pos")));
      return &mp;
    }

    case Type::MwndList: {
      static const auto mp = make_flatmap<Builder>(
          // TODO: Handle ELM_UP = -5
          id[-1] | b_index_array(Type::Mwnd),
          id[1] | b(Type::None, Call("close")),
          id[2] | b(Type::None, Call("close_wait")),
          id[3] | b(Type::None, Call("close_nowait")));
      return &mp;
    }

    case Type::Mwnd: {
      static const auto mp = make_flatmap<Builder>(
          // TODO: Handle ELM_UP = -5
          id[0] | callable(fn("set_waku")[2](Type::Int, Type::Int),
                           fn("set_waku")[1](Type::Int), fn("set_waku")[any]()),
          id[79] | b(Type::None, Call("init_waku_file")),
          id[78] | callable(fn("set_waku_file")[any](Type::String)),
          id[80] | b(Type::String, Call("get_waku_file")),
          id[82] | b(Type::None, Call("init_filter_file")),
          id[81] | callable(fn("set_filter_file")[any](Type::String)),
          id[83] | b(Type::String, Call("get_filter_file")),
          id[1] | b(Type::None, Call("open")),
          id[15] | b(Type::None, Call("open_wait")),
          id[16] | b(Type::None, Call("open_nowait")),
          id[65] | b(Type::Int, Call("check_open")),
          id[2] | b(Type::None, Call("close")),
          id[13] | b(Type::None, Call("close_wait")),
          id[14] | b(Type::None, Call("close_nowait")),
          id[64] | b(Type::None, Call("end_close")),
          id[49] | b(Type::None, Call("msg_block")),
          id[59] | b(Type::None, Call("msg_pp_block")),
          id[3] | b(Type::None, Call("clear")),
          id[55] | b(Type::None, Call("novel_clear")),
          id[4] |
              callable(fn("print")[0](Type::Int), fn("print")[1](Type::String)),
          id[57] |
              callable(fn("overflow_print")[0](Type::Int, Type::Int, Type::Int),
                       fn("print")[1](Type::String, Type::Int, Type::Int)),
          id[63] | callable(fn("overflow_name")[0](Type::Int),
                            fn("overflow_name")[1](Type::String)),
          id[12] | callable(fn("ruby_end")[0](),
                            fn("ruby_start")[any](Type::String)),
          id[18] | b(Type::None, Call("msg_wait")),
          id[19] | b(Type::None, Call("pp")), id[20] | b(Type::None, Call("r")),
          id[54] | b(Type::None, Call("page")),
          id[6] | b(Type::None, Call("nl")),
          id[17] | b(Type::None, Call("nil")),
          id[56] | b(Type::None, Call("indent")),
          id[28] | b(Type::None, Call("clear_indent")),
          id[31] | b(Type::None, Call("enable_multi_msg")),
          id[29] | b(Type::None, Call("next_msg")),
          id[58] | callable(fn("set_slide_msg")[any](Type::Int)),
          id[60] | b(Type::None, Call("set_slide_msg")),
          id[61] | b(Type::None, Call("slide_msg")),

          // SEL
          id[5] | callable(fn("sel")[any]()),
          id[51] | callable(fn("sel_cancel")[any]()),
          id[50] | callable(fn("selmsg")[any]()),
          id[52] | callable(fn("selmsg_cancel")[any]()),
          id[84] | callable(fn("rep_pos_default")[0](),
                            fn("rep_pos")[1](Type::Int, Type::Int)),
          id[7] | callable(fn("size_default")[0](),
                           fn("size")[1](Type::Int, Type::Int)),
          id[8] | callable(fn("color_default")[0](),
                           fn("color")[1](Type::Int, Type::Int)),
          id[86] | callable(fn("msgbtn")[0](), fn("msgbtn")[1](Type::Int),
                            fn("msgbtn")[2](Type::Int, Type::Int),
                            fn("msgbtn")[3](Type::Int, Type::Int, Type::Int,
                                            Type::Int)),
          id[85] | callable(fn("set_namae")[any](Type::String)),
          id[9] |
              callable(fn("koe")[1](Type::Int, Type::Int, kw_arg(0, Type::Int)),
                       fn("koe")[0](Type::Int, kw_arg(0, Type::Int)),
                       fn("koe")[any](kw_arg(0, Type::Int))),
          id[26] |
              callable(fn("koe_play_wait")[1](Type::Int, Type::Int,
                                              kw_arg(0, Type::Int)),
                       fn("koe_play_wait")[0](Type::Int, kw_arg(0, Type::Int)),
                       fn("koe_play_wait")[any](kw_arg(0, Type::Int))),
          id[27] | callable(fn("koe_play_wait_key")[1](Type::Int, Type::Int,
                                                       kw_arg(0, Type::Int)),
                            fn("koe_play_wait_key")[0](Type::Int,
                                                       kw_arg(0, Type::Int)),
                            fn("koe_play_wait_key")[any](kw_arg(0, Type::Int))),
          id[22] | b(Type::None, Call("clear_face")),
          id[21] | callable(fn("set_face")[0](Type::String),
                            fn("set_face")[1](Type::Int, Type::String)),
          id[10] |
              callable(fn("get_layer")[0](), fn("set_layer")[1](Type::Int)),
          id[11] |
              callable(fn("get_world")[0](), fn("set_world")[1](Type::Int)),
          id[32] | b(Type::ObjList, Member("button")),
          id[53] | b(Type::ObjList, Member("face")),
          id[30] | b(Type::ObjList, Member("object")),

          // parameter
          id[66] | b(Type::None, Call("init_window_pos")),
          id[67] | b(Type::None, Call("init_window_size")),
          id[74] | b(Type::None, Call("init_window_moji_cnt")),
          id[68] | callable(fn("set_window_pos")[any](Type::Int, Type::Int)),
          id[69] | callable(fn("set_window_size")[any](Type::Int, Type::Int)),
          id[75] |
              callable(fn("set_window_moji_cnt")[any](Type::Int, Type::Int)),
          id[70] | b(Type::Int, Call("get_window_pos_x")),
          id[71] | b(Type::Int, Call("get_window_pos_y")),
          id[72] | b(Type::Int, Call("get_window_size_x")),
          id[73] | b(Type::Int, Call("get_window_size_y")),
          id[77] | b(Type::Int, Call("get_window_moji_cnt_x")),
          id[76] | b(Type::Int, Call("get_window_moji_cnt_y")),
          id[41] | b(Type::None, Call("init_open_anime_type")),
          id[42] | b(Type::None, Call("init_open_anime_time")),
          id[43] | b(Type::None, Call("init_close_anime_type")),
          id[44] | b(Type::None, Call("init_close_anime_time")),
          id[37] | callable(fn("set_open_anime_type")[any](Type::Int)),
          id[34] | callable(fn("set_open_anime_time")[any](Type::Int)),
          id[45] | callable(fn("set_close_anime_type")[any](Type::Int)),
          id[35] | callable(fn("set_close_anime_time")[any](Type::Int)),
          id[48] | b(Type::Int, Call("get_open_anime_type")),
          id[47] | b(Type::Int, Call("get_open_anime_time")),
          id[38] | b(Type::Int, Call("get_close_anime_type")),
          id[46] | b(Type::Int, Call("get_close_anime_time")),
          id[36] | b(Type::Int, Call("get_default_open_anime_type")),
          id[33] | b(Type::Int, Call("get_default_open_anime_time")),
          id[40] | b(Type::Int, Call("get_default_close_anime_type")),
          id[39] | b(Type::Int, Call("get_default_close_anime_time")));
      return &mp;
    }

    case Type::Pcm: {
      static const auto mp =
          make_flatmap<Builder>(id[0] | callable(fn("play")[any](Type::String)),
                                id[1] | b(Type::None, Call("stop")));
      return &mp;
    }

    case Type::PcmchList: {
      static const auto mp =
          make_flatmap<Builder>(id[-1] | b_index_array(Type::Pcmch));
      return &mp;
    }

    case Type::Pcmch: {
      static const auto mp = make_flatmap<Builder>(
          id[0] |
              callable(
                  fn("play")[1](Type::String, Type::Int, kw_arg(0, Type::Int),
                                kw_arg(1, Type::Int), kw_arg(2, Type::Int),
                                kw_arg(3, Type::Int), kw_arg(4, Type::Int),
                                kw_arg(5, Type::Int), kw_arg(6, Type::Int),
                                kw_arg(7, Type::String), kw_arg(8, Type::Int),
                                kw_arg(9, Type::Int), kw_arg(10, Type::String),
                                kw_arg(11, Type::Int)),
                  fn("play")[0](Type::String, kw_arg(0, Type::Int),
                                kw_arg(1, Type::Int), kw_arg(2, Type::Int),
                                kw_arg(3, Type::Int), kw_arg(4, Type::Int),
                                kw_arg(5, Type::Int), kw_arg(6, Type::Int),
                                kw_arg(7, Type::String), kw_arg(8, Type::Int),
                                kw_arg(9, Type::Int), kw_arg(10, Type::String),
                                kw_arg(11, Type::Int)),
                  fn("play")[2](kw_arg(0, Type::Int), kw_arg(1, Type::Int),
                                kw_arg(2, Type::Int), kw_arg(3, Type::Int),
                                kw_arg(4, Type::Int), kw_arg(5, Type::Int),
                                kw_arg(6, Type::Int), kw_arg(7, Type::String),
                                kw_arg(8, Type::Int), kw_arg(9, Type::Int),
                                kw_arg(10, Type::String),
                                kw_arg(11, Type::Int))),
          id[2] | callable(fn("play_loop")[0](Type::String),
                           fn("play_loop")[1](Type::String, Type::Int)),
          id[1] | callable(fn("play_wait")[0](Type::String),
                           fn("play_wait")[1](Type::String, Type::Int)),
          id[11] |
              callable(
                  fn("ready")[0](Type::String, kw_arg(0, Type::Int),
                                 kw_arg(3, Type::Int), kw_arg(4, Type::Int),
                                 kw_arg(5, Type::Int), kw_arg(6, Type::Int),
                                 kw_arg(7, Type::String), kw_arg(8, Type::Int),
                                 kw_arg(9, Type::Int), kw_arg(10, Type::String),
                                 kw_arg(11, Type::Int)),
                  fn("ready")[any](
                      kw_arg(0, Type::Int), kw_arg(3, Type::Int),
                      kw_arg(4, Type::Int), kw_arg(5, Type::Int),
                      kw_arg(6, Type::Int), kw_arg(7, Type::String),
                      kw_arg(8, Type::Int), kw_arg(9, Type::Int),
                      kw_arg(10, Type::String), kw_arg(11, Type::Int))),
          id[16] | callable(fn("ready_loop")[0](Type::String),
                            fn("ready_loop")[1](Type::String, Type::Int)),
          id[5] | callable(fn("stop")[0](), fn("stop")[1](Type::Int)),
          id[10] | callable(fn("pause")[0](), fn("pause")[1](Type::Int)),
          id[9] | callable(fn("resume")[1](Type::Int, kw_arg(0, Type::Int)),
                           fn("resume")[0](kw_arg(0, Type::Int))),
          id[17] |
              callable(fn("resume_wait")[0](), fn("resume_wait")[1](Type::Int)),
          id[3] | b(Type::None, Call("wait")),
          id[6] | b(Type::Int, Call("wait_key")),
          id[8] | b(Type::None, Call("wait_fade")),
          id[7] | b(Type::Int, Call("wait_fade_key")),
          id[4] | b(Type::Int, Call("check")),
          id[13] | callable(fn("set_volume")[0](Type::Int),
                            fn("set_volume")[1](Type::Int, Type::Int)),
          id[14] |
              callable(fn("set_vol_max")[0](), fn("set_vol_max")[1](Type::Int)),
          id[15] |
              callable(fn("set_vol_min")[0](), fn("set_vol_min")[1](Type::Int)),
          id[12] | b(Type::Int, Call("get_volume")));
      return &mp;
    }

    [[unlikely]]
    default:
      return nullptr;
  }
}

// ------------------------------------------------------------------------------
// ==============================================================================
// ElementParser public interface
ElementParser::ElementParser(std::unique_ptr<Context> ctx)
    : ctx_(std::move(ctx)) {}
ElementParser::~ElementParser() = default;

AccessChain ElementParser::Parse(ElementCode& elm) {
  const int first = elm.At<int>(0);
  const int flag = first >> 24;
  const size_t idx = first ^ (flag << 24);

  AccessChain result;
  if (flag == USER_COMMAND_FLAG)
    result = resolve_usrcmd(elm, idx);
  else if (flag == USER_PROPERTY_FLAG)
    result = resolve_usrprop(elm, idx);
  else
    result = resolve_element(elm);

  if (elm.bind_ctx.Empty())  // implicit
    elm.force_bind = false;
  if (elm.force_bind) {
    ctx_->Warn("[ElementParser] bind ignored: " + elm.bind_ctx.ToDebugString());
  }

  return result;
}

AccessChain ElementParser::resolve_usrcmd(ElementCode& elm, size_t idx) {
  Usrcmd usrcmd;

  const libsiglus::Command* cmd = nullptr;
  if (idx < ctx_->GlobalCommands().size())
    cmd = &ctx_->GlobalCommands()[idx];
  else
    cmd = &ctx_->SceneCommands()[idx - ctx_->GlobalCommands().size()];

  usrcmd.scene = cmd->scene_id;
  usrcmd.entry = cmd->offset;
  usrcmd.name = cmd->name;
  usrcmd.arguments = std::move(elm.bind_ctx);

  elm.force_bind = false;
  return AccessChain{.root = std::move(usrcmd), .nodes = {}};
}

AccessChain ElementParser::resolve_usrprop(ElementCode& elm, size_t idx) {
  elm::Usrprop root;
  Type root_type;

  if (idx < ctx_->GlobalProperties().size()) {
    const auto& incprop = ctx_->GlobalProperties()[idx];
    root.name = incprop.name;
    root_type = incprop.form;
    root.scene = -1;  // global
    root.idx = idx;
  } else {
    idx -= ctx_->GlobalProperties().size();
    const auto& usrprop = ctx_->SceneProperties()[idx];
    root.name = usrprop.name;
    root_type = usrprop.form;
    root.scene = ctx_->SceneId();
    root.idx = idx;
  }

  return make_chain(root_type, std::move(root), elm, 1);
}

AccessChain ElementParser::resolve_element(ElementCode& elm) {
  auto elm_iv = elm.IntegerView();
  int root = elm_iv.front();

  switch (root) {
      // ====== Memory Banks ======
    case 25:  // A
      return make_chain(Type::IntList, elm::Sym("A"), elm, 1);
    case 26:  // B
      return make_chain(Type::IntList, elm::Sym("B"), elm, 1);
    case 27:  // C
      return make_chain(Type::IntList, elm::Sym("C"), elm, 1);
    case 28:  // D
      return make_chain(Type::IntList, elm::Sym("D"), elm, 1);
    case 29:  // E
      return make_chain(Type::IntList, elm::Sym("E"), elm, 1);
    case 30:  // F
      return make_chain(Type::IntList, elm::Sym("F"), elm, 1);
    case 137:  // X
      return make_chain(Type::IntList, elm::Sym("X"), elm, 1);
    case 31:  // G
      return make_chain(Type::IntList, elm::Sym("G"), elm, 1);
    case 32:  // Z
      return make_chain(Type::IntList, elm::Sym("Z"), elm, 1);

    case 34:  // S
      return make_chain(Type::StrList, elm::Sym("S"), elm, 1);
    case 35:  // M
      return make_chain(Type::StrList, elm::Sym("M"), elm, 1);
    case 106:  // NAMAE_LOCAL
      return make_chain(Type::StrList, elm::Sym("LN"), elm, 1);
    case 107:  // NAMAE_GLOBAL
      return make_chain(Type::StrList, elm::Sym("GN"), elm, 1);

      // ====== CUR_CALL (Special Case) ======
    case 83: {  // CUR_CALL
      const int elmcall = elm_iv[1];
      if ((elmcall >> 24) == 0x7d) {
        auto id = (elmcall ^ (0x7d << 24));
        return make_chain(ctx_->CurcallArgs()[id], elm::Arg(id), elm, 2);
      }

      else if (elmcall == 0)
        return make_chain(Type::IntList, elm::Sym("L"), elm, 2);

      else if (elmcall == 1)
        return make_chain(Type::StrList, elm::Sym("K"), elm, 2);
    } break;

      // ====== Sound ======
    case 42:  // BGM
      return make_chain(Type::Bgm, elm::Sym("bgm"), elm, 1);
    case 123:  // BGMTABLE
      return make_chain(Type::BgmTable, elm::Sym("bgm_table"), elm, 1);
    case 43:
      return make_chain(Type::Pcm, elm::Sym("pcm"), elm, 1);
    case 44:
      return make_chain(Type::PcmchList, elm::Sym("pcmch_list"), elm, 1);

      // ====== SEL ======
      // some needs kidoku flag
    case 19:   // SEL
    case 101:  // SEL_CANCEL
      ctx_->ReadKidoku();
      break;

    case 100:  // SELMSG
    case 102:  // SELMSG_CANCEL
      ctx_->ReadKidoku();
      break;

    case 76:   // SELBTN
    case 77:   // SELBTN_READY
    case 126:  // SELBTN_CANCEL
    case 128:  // SELBTN_CANCEL_READY
    case 127:  // SELBTN_START
    {
      if (root == 76 || root == 126 || root == 127)
        ctx_->ReadKidoku();
      break;
    }

      // ====== Title ======
    case 74: {  // SET_TITLE
      Call set_title;
      set_title.name = "set_title";
      set_title.args = {elm.bind_ctx.arg[0]};
      elm.force_bind = false;
      return AccessChain{.root = std::monostate(),
                         .nodes = {std::move(set_title)}};
    }
    case 75: {  // GET_TITLE
      Call get_title;
      get_title.name = "get_title";
      AccessChain chain{.root = std::monostate(),
                        .nodes = {Node(Type::String, std::move(get_title))}};
      return make_chain(std::move(chain), elm,
                        std::span(elm.code.begin() + 1, elm.code.end()));
    }

      // ====== MWND ======
    case 22: {  // SET_WAKU
      elm.code.front() = Value(Integer(0));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 9: {  // OPEN
      elm.code.front() = Value(Integer(1));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 58: {  // OPEN_WAIT
      elm.code.front() = Value(Integer(15));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 59: {  // OPEN_NOWAIT
      elm.code.front() = Value(Integer(16));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 10: {  // CLOSE
      elm.code.front() = Value(Integer(2));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 56: {  // CLOSE_WAIT
      elm.code.front() = Value(Integer(13));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 57: {  // CLOSE_NOWAIT
      elm.code.front() = Value(Integer(14));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 125: {  // END_CLOSE
      elm.code.front() = Value(Integer(64));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 84: {  // MSG_BLOCK
      elm.code.front() = Value(Integer(49));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 121: {  // MSG_PP_BLOCK
      elm.code.front() = Value(Integer(59));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 11: {  // CLEAR
      elm.code.front() = Value(Integer(3));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 156: {  // SET_NAMAE
      elm.code.front() = Value(Integer(85));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 12: {  // PRINT
      elm.code.front() = Value(Integer(4));
      auto result = make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
      result.kidoku = ctx_->ReadKidoku();
      return result;
    }
    case 61: {  // RUBY
      elm.code.front() = Value(Integer(12));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 47: {  // MSGBTN
      elm.code.front() = Value(Integer(86));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 15: {  // NL
      elm.code.front() = Value(Integer(6));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 62: {  // NLI
      elm.code.front() = Value(Integer(17));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 119: {  // INDENT
      elm.code.front() = Value(Integer(56));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 94: {  // CLEAR_INDENT
      elm.code.front() = Value(Integer(28));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 21: {  // WAIT_MSG
      elm.code.front() = Value(Integer(18));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 13: {  // PP
      elm.code.front() = Value(Integer(19));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 14: {  // R
      elm.code.front() = Value(Integer(20));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 115: {  // PAGE
      elm.code.front() = Value(Integer(54));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 151: {  // REP_POS
      elm.code.front() = Value(Integer(84));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 16: {  // SIZE
      elm.code.front() = Value(Integer(7));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 17: {  // COLOR
      elm.code.front() = Value(Integer(8));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 95: {  // MULTI_MSG
      elm.code.front() = Value(Integer(31));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 93: {  // NEXT_MSG
      elm.code.front() = Value(Integer(29));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 120: {  // START_SLIDE_MSG
      elm.code.front() = Value(Integer(58));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 122: {  // END_SLIDE_MSG
      elm.code.front() = Value(Integer(60));
      return make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
    }
    case 18:  // KOE
      elm.code.front() = Value(Integer(9));
      goto KOE;

    case 90:  // KOE_PLAY_WAIT
      elm.code.front() = Value(Integer(26));
      goto KOE;

    case 91:  // KOE_PLAY_WAIT_KEY
      elm.code.front() = Value(Integer(27));
      goto KOE;
    KOE: {
      auto result = make_chain(Type::Mwnd, elm::Sym("mwnd"), elm, 0);
      result.kidoku = ctx_->ReadKidoku();
      return result;
    }

      // ====== Uncategorized ======
    case 5: {  // FARCALL
      auto& bind = elm.bind_ctx;

      Farcall farcall;
      if (Typeof(bind.arg[0]) != Type::String)
        ctx_->Warn("[Farcall] expected string, but got: " +
                   ToString(bind.arg[0]));
      farcall.scn_name = bind.arg[0];

      if (bind.overload_id == 1) {  // additionally has zlabel and arguments
        if (Typeof(bind.arg[1]) != Type::Int)
          ctx_->Warn("[Farcall] expected int, but got: " +
                     ToString(bind.arg[1]));

        farcall.zlabel = bind.arg[1];
        for (auto& arg : std::views::drop(bind.arg, 2)) {
          switch (Typeof(arg)) {
            case Type::Int:
              farcall.intargs.emplace_back(std::move(arg));
              break;
            case Type::String:
              farcall.strargs.emplace_back(std::move(arg));
              break;
            default:
              throw std::runtime_error(
                  "Farcall: Expected int or str, but got " + ToString(arg));
          }
        }
      }

      elm.force_bind = false;
      return AccessChain{.root = std::move(farcall)};
    }

    case 49:  // STAGE
      return make_chain(Type::StageList, elm::Sym("stage"), elm, 1);
    case 37:  // BACK
      return make_chain(Type::Stage, elm::Sym("stage_back"), elm, 1);
    case 38:  // FRONT
      return make_chain(Type::Stage, elm::Sym("stage_front"), elm, 1);
    case 73:  // NEXT
      return make_chain(Type::Stage, elm::Sym("stage_next"), elm, 1);

    case 65:  // EXCALL
      return make_chain(Type::Excall, elm::Sym("excall"), elm, 1);

    case 135:  // MASK
      return make_chain(Type::MaskList, elm::Sym("mask"), elm, 1);

    case 63:  // SYSCOM
      return make_chain(Type::Syscom, elm::Sym("syscom"), elm, 1);
    case 64:  // SCRIPT
      return make_chain(Type::Script, elm::Sym("script"), elm, 1);

    case 54:    // WAIT
    case 55: {  // WAIT_KEY
      Wait wait;
      wait.interruptable = root == 55;
      if (Typeof(elm.bind_ctx.arg[0]) != Type::Int)
        ctx_->Warn("[Wait] expected int, but got: " +
                   ToString(elm.bind_ctx.arg[0]));
      wait.time_ms = elm.bind_ctx.arg[0];
      elm.force_bind = false;
      return AccessChain{.root = std::move(wait)};
    }

    case 92:  // SYSTEM
      return make_chain(Type::System, elm::Sym("system"), elm, 1);

    case 40:  // COUNTER
      return make_chain(Type::CounterList, elm::Sym("counter"), elm, 1);

    case 79:  // FRAME_ACTION
      return make_chain(Type::FrameAction, elm::Sym("frame_action"), elm, 1);
    case 53:  // FRAME_ACTION_CH
      return make_chain(Type::FrameActionList, elm::Sym("frame_action_ch"), elm,
                        1);

    case 20:  // MOVIE
      return make_chain(Type::Movie, elm::Sym("mov"), elm, 1);

    [[unlikely]]
    default: {
      std::string msg = "[ElementParser] Unable to parse element: ";
      for (const Value& it : elm.code)
        msg += '<' + ToString(it) + '>';
      ctx_->Warn(std::move(msg));
    } break;
  }

  return AccessChain();
}

AccessChain ElementParser::make_chain(AccessChain result,
                                      ElementCode& elm,
                                      std::span<const Value> elmcode) {
  result.nodes.reserve(elmcode.size());

  flat_map<Builder> const* mp;
  Builder::Ctx ctx{
      .elm = elm,
      .elmcode = elmcode,
      .chain = result,
      .Warn = [&](std::string msg) { this->ctx_->Warn(std::move(msg)); }};
  while ((mp = elm::GetMethodMap(result.GetType())) != nullptr) {
    if (!mp || elmcode.empty())
      break;
    if (!std::holds_alternative<Integer>(elmcode.front()))
      break;
    const int key = AsInt(elmcode.front());
    if (!mp->contains(key))
      break;

    mp->at(key).Build(ctx);
  }

  if (!elmcode.empty()) {
    std::string msg = "[ElementParser] leftovers: ";
    msg += Join(",", std::views::all(elmcode) |
                         std::views::transform(
                             [](const Value& v) { return ToString(v); }));
    ctx_->Warn(std::move(msg));
  }

  return result;
}

AccessChain ElementParser::make_chain(Type root_type,
                                      Root::var_t root_node,
                                      ElementCode& elm,
                                      size_t subidx) {
  Root root(root_type, std::move(root_node));
  AccessChain result{.root = std::move(root), .nodes = {}};
  return make_chain(std::move(result), elm,
                    std::span{elm.code}.subspan(subidx));
}

}  // namespace libsiglus::elm
