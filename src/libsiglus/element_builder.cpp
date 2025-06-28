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

#include "libsiglus/element_builder.hpp"

#include "libsiglus/callable_builder.hpp"
#include "log/domain_logger.hpp"
#include "utilities/flat_map.hpp"

namespace libsiglus::elm {
using namespace libsiglus::elm::callable_builder;

class Builder {
 public:
  struct Ctx {
    std::span<const Value>& elmcode;
    AccessChain& chain;
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
    ctx.chain.nodes.emplace_back(t, Subscript{ctx.elmcode[1]});
    ctx.elmcode = ctx.elmcode.subspan(2);
  });
}
template <typename... Ts>
inline static Builder callable(Ts&&... params) {
  return b(Type::Callable, make_callable(std::forward<Ts>(params)...));
}
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
          id[10] | callable(fn("init")[any]().ret(Type::None)),
          id[2] | callable(fn("resize")[any](Type::Int).ret(Type::None)),
          id[9] | callable(fn("size")[any]().ret(Type::Int)),
          id[8] | callable(fn("fill")[0](Type::Int, Type::Int).ret(Type::None),
                           fn("fill")[any](Type::Int, Type::Int, Type::Int)
                               .ret(Type::None)),
          id[1] | callable(fn("Set")[any](Type::Int, va_arg(Type::Int))
                               .ret(Type::None)));
      return &mp;
    }

    case Type::IntEventList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | b_index_array(Type::IntEvent),
          id[1] | callable(fn("resize")[any](Type::Int).ret(Type::None)));
      return &mp;
    }
    case Type::IntEvent: {
      static const auto mp = make_flatmap<Builder>(
          id[0] | callable(fn("set")[any](Type::Int, Type::Int, Type::Int,
                                          Type::Int, kw_arg(0, Type::Int))
                               .ret(Type::None)),
          id[7] | callable(fn("set_real")[any](Type::Int, Type::Int, Type::Int,
                                               Type::Int, kw_arg(0, Type::Int))
                               .ret(Type::None)),
          id[1] | callable(fn("loop")[any](Type::Int, Type::Int, Type::Int,
                                           Type::Int, Type::Int)
                               .ret(Type::None)),
          id[8] | callable(fn("loop_real")[any](Type::Int, Type::Int, Type::Int,
                                                Type::Int, Type::Int)
                               .ret(Type::None)),
          id[2] | callable(fn("turn")[any](Type::Int, Type::Int, Type::Int,
                                           Type::Int, Type::Int)
                               .ret(Type::None)),
          id[9] | callable(fn("turn_real")[any](Type::Int, Type::Int, Type::Int,
                                                Type::Int, Type::Int)
                               .ret(Type::None)),
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
          id[2] | callable(fn("resize")[any](Type::Int).ret(Type::None)),
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
          id[14] | b(Type::Invalid, Member("calendar")),
          id[15] | b(Type::Int, Member("time")),
          id[0] | b(Type::Int, Member("window_active")),
          id[13] | b(Type::Int, Member("is_debug")),
          id[1] | b(Type::None, Member("shell_openfile")),
          id[5] | b(Type::None, Member("openurl")),
          id[6] | b(Type::Int, Member("check_file_exist")),
          id[12] | b(Type::Int, Member("check_file_exist")),
          id[2] | b(Type::None, Member("check_dummy")),
          id[21] | b(Type::None, Member("clear_dummy")),
          id[17] | b(Type::Int, Member("msgbox_ok")),
          id[18] | b(Type::Int, Member("msgbox_okcancel")),
          id[19] | b(Type::Int, Member("msgbox_yn")),
          id[20] | b(Type::Int, Member("msgbox_yncancel")),
          id[4] | b(Type::String, Member("get_chihayabench")),
          id[3] | b(Type::None, Member("open_chihayabench")),
          id[16] | b(Type::None, Member("get_lang")));
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
      static const auto mp =
          make_flatmap<Builder>(id[1] | b(Type::None, Member("start")),
                                id[3] | b(Type::None, Member("start_real")),
                                id[2] | b(Type::None, Member("end")),
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
      return nullptr;
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
      static const auto mp = make_flatmap<Builder>(
          id[56] | b(Type::Int, Member("wipe_copy")),
          id[92] | b(Type::Int, Member("wipe_erase")),
          id[139] | b(Type::Int, Member("click_disable")),
          id[0] | b(Type::Int, Member("disp")),
          id[1] | b(Type::Int, Member("patno")),
          id[44] | b(Type::Int, Member("world")),
          id[55] | b(Type::Int, Member("order")),
          id[2] | b(Type::Int, Member("layer")),
          id[3] | b(Type::Int, Member("x")), id[4] | b(Type::Int, Member("y")),
          id[5] | b(Type::Int, Member("z")),
          id[54] | b(Type::Int, Member("x_rep")),
          id[63] | b(Type::Int, Member("y_rep")),
          id[110] | b(Type::Int, Member("z_rep")),
          id[48] | b(Type::Int, Member("set_pos")),
          id[6] | b(Type::Int, Member("center_x")),
          id[7] | b(Type::Int, Member("center_y")),
          id[8] | b(Type::Int, Member("center_z")),
          id[158] | b(Type::Int, Member("set_center")),
          id[9] | b(Type::Int, Member("center_rep_x")),
          id[10] | b(Type::Int, Member("center_rep_y")),
          id[11] | b(Type::Int, Member("center_rep_z")),
          id[159] | b(Type::Int, Member("set_center_rep")),
          id[12] | b(Type::Int, Member("scale_x")),
          id[13] | b(Type::Int, Member("scale_y")),
          id[14] | b(Type::Int, Member("scale_z")),
          id[49] | b(Type::Int, Member("set_scale")),
          id[15] | b(Type::Int, Member("rotate_x")),
          id[16] | b(Type::Int, Member("rotate_y")),
          id[17] | b(Type::Int, Member("rotate_z")),
          id[50] | b(Type::Int, Member("set_rotate")),
          id[18] | b(Type::Int, Member("clip_use")),
          id[19] | b(Type::Int, Member("clip_left")),
          id[20] | b(Type::Int, Member("clip_top")),
          id[21] | b(Type::Int, Member("clip_right")),
          id[22] | b(Type::Int, Member("clip_bottom")),
          id[160] | b(Type::Int, Member("set_clip")),
          id[149] | b(Type::Int, Member("src_clip_use")),
          id[150] | b(Type::Int, Member("src_clip_left")),
          id[151] | b(Type::Int, Member("src_clip_top")),
          id[152] | b(Type::Int, Member("src_clip_right")),
          id[153] | b(Type::Int, Member("src_clip_bottom")),
          id[161] | b(Type::Int, Member("set_src_clip")),
          id[27] | b(Type::Int, Member("tr")),
          id[141] | b(Type::Int, Member("tr_rep")),
          id[28] | b(Type::Int, Member("mono")),
          id[29] | b(Type::Int, Member("reverse")),
          id[30] | b(Type::Int, Member("bright")),
          id[31] | b(Type::Int, Member("dark")),
          id[32] | b(Type::Int, Member("color_r")),
          id[33] | b(Type::Int, Member("color_g")),
          id[34] | b(Type::Int, Member("color_b")),
          id[23] | b(Type::Int, Member("color_rate")),
          id[57] | b(Type::Int, Member("color_add_r")),
          id[58] | b(Type::Int, Member("color_add_g")),
          id[59] | b(Type::Int, Member("color_add_b")),
          id[145] | b(Type::Int, Member("mask_no")),
          id[109] | b(Type::Int, Member("tonecurve_no")),
          id[146] | b(Type::Int, Member("culling")),
          id[147] | b(Type::Int, Member("alpha_test")),
          id[148] | b(Type::Int, Member("alpha_blend")),
          id[46] | b(Type::Int, Member("blend")),
          id[168] | b(Type::Int, Member("light_no")),
          id[144] | b(Type::Int, Member("fog_use")),
          id[90] | b(Type::Int, Member("patno_eve")),
          id[51] | b(Type::Int, Member("x_eve")),
          id[64] | b(Type::Int, Member("y_eve")),
          id[65] | b(Type::Int, Member("z_eve")),
          id[112] | b(Type::Int, Member("x_rep_eve")),
          id[113] | b(Type::Int, Member("y_rep_eve")),
          id[114] | b(Type::Int, Member("z_rep_eve")),
          id[77] | b(Type::Int, Member("center_x_eve")),
          id[78] | b(Type::Int, Member("center_y_eve")),
          id[79] | b(Type::Int, Member("center_z_eve")),
          id[80] | b(Type::Int, Member("center_rep_x_eve")),
          id[81] | b(Type::Int, Member("center_rep_y_eve")),
          id[82] | b(Type::Int, Member("center_rep_z_eve")),
          id[67] | b(Type::Int, Member("scale_x_eve")),
          id[68] | b(Type::Int, Member("scale_y_eve")),
          id[66] | b(Type::Int, Member("scale_z_eve")),
          id[69] | b(Type::Int, Member("rotate_x_eve")),
          id[70] | b(Type::Int, Member("rotate_y_eve")),
          id[71] | b(Type::Int, Member("rotate_z_eve")),
          id[105] | b(Type::Int, Member("clip_left_eve")),
          id[106] | b(Type::Int, Member("clip_top_eve")),
          id[107] | b(Type::Int, Member("clip_right_eve")),
          id[108] | b(Type::Int, Member("clip_bottom_eve")),
          id[154] | b(Type::Int, Member("src_clip_left_eve")),
          id[155] | b(Type::Int, Member("src_clip_top_eve")),
          id[156] | b(Type::Int, Member("src_clip_right_eve")),
          id[157] | b(Type::Int, Member("src_clip_bottom_eve")),
          id[72] | b(Type::Int, Member("tr_eve")),
          id[140] | b(Type::Int, Member("tr_rep_eve")),
          id[73] | b(Type::Int, Member("mono_eve")),
          id[74] | b(Type::Int, Member("reverse_eve")),
          id[75] | b(Type::Int, Member("bright_eve")),
          id[76] | b(Type::Int, Member("dark_eve")),
          id[87] | b(Type::Int, Member("color_r_eve")),
          id[88] | b(Type::Int, Member("color_g_eve")),
          id[89] | b(Type::Int, Member("color_b_eve")),
          id[83] | b(Type::Int, Member("color_rate_eve")),
          id[84] | b(Type::Int, Member("color_add_r_eve")),
          id[85] | b(Type::Int, Member("color_add_g_eve")),
          id[86] | b(Type::Int, Member("color_add_b_eve")),
          id[91] | Builder([](Builder::Ctx& ctx) {
            const int peek = AsInt(ctx.elmcode[1]);
            ctx.chain.nodes.emplace_back(Type::Invalid, Member("alleve"));
            switch (peek) {
              case 0:  // ALLEVENT_END
                ctx.chain.nodes.emplace_back(Type::None, Member("end"));
                return;
              case 1:
                ctx.chain.nodes.emplace_back(Type::None, Member("wait"));
                return;
              case 2:
                ctx.chain.nodes.emplace_back(Type::Int, Member("check"));
                return;
              default:
                ctx.chain.nodes.emplace_back(Type::Invalid, Member("???"));
                return;
            }
          }),
          id[93] | b(Type::ObjList, Member("child")),
          id[35] | b(Type::None, Call("init")),
          id[36] | b(Type::None, Call("free")),
          id[37] | b(Type::None, Call("init_param")));
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

    default:
      return nullptr;
  }
}

// -----------------------------------------------------------------------
AccessChain MakeChain(ElementCode const& elmcode, BindCtx bind) {
  using namespace libsiglus::elm::callable_builder;

  auto elm = elmcode.IntegerView();
  int root = elm.front();

  switch (root) {
    // ====== Memory Banks ======
    case 25:  // A
      return elm::MakeChain(Type::IntList, elm::Sym("A"), elmcode, 1);
    case 26:  // B
      return elm::MakeChain(Type::IntList, elm::Sym("B"), elmcode, 1);
    case 27:  // C
      return elm::MakeChain(Type::IntList, elm::Sym("C"), elmcode, 1);
    case 28:  // D
      return elm::MakeChain(Type::IntList, elm::Sym("D"), elmcode, 1);
    case 29:  // E
      return elm::MakeChain(Type::IntList, elm::Sym("E"), elmcode, 1);
    case 30:  // F
      return elm::MakeChain(Type::IntList, elm::Sym("F"), elmcode, 1);
    case 137:  // X
      return elm::MakeChain(Type::IntList, elm::Sym("X"), elmcode, 1);
    case 31:  // G
      return elm::MakeChain(Type::IntList, elm::Sym("G"), elmcode, 1);
    case 32:  // Z
      return elm::MakeChain(Type::IntList, elm::Sym("Z"), elmcode, 1);

    case 34:  // S
      return elm::MakeChain(Type::StrList, elm::Sym("S"), elmcode, 1);
    case 35:  // M
      return elm::MakeChain(Type::StrList, elm::Sym("M"), elmcode, 1);
    case 106:  // NAMAE_LOCAL
      return elm::MakeChain(Type::StrList, elm::Sym("LN"), elmcode, 1);
    case 107:  // NAMAE_GLOBAL
      return elm::MakeChain(Type::StrList, elm::Sym("GN"), elmcode, 1);

      // ====== Title ======
    case 74: {  // SET_TITLE
      Call set_title;
      set_title.name = "set_title";
      set_title.args = {bind.arglist[0]};
      return AccessChain{.root = std::monostate(),
                         .nodes = {std::move(set_title)}};
    }

    case 75:  // GET_TITLE
      return elm::MakeChain(Type::String, elm::Sym("get_title"), elmcode, 1);

      // ====== Uncategorized ======
    case 5: {  // FARCALL
      Farcall farcall;
      farcall.scn_name = AsStr(bind.arglist[0]);

      if (bind.overload == 1) {  // additionally has zlabel and arguments
        farcall.zlabel = AsInt(bind.arglist[1]);
        for (auto& arg : std::views::drop(bind.arglist, 2)) {
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

      return AccessChain{.root = std::move(farcall)};
    }

    case 49:  // STAGE
      return elm::MakeChain(Type::StageList, elm::Sym("stage"), elmcode, 1);
    case 37:  // BACK
      return elm::MakeChain(Type::Stage, elm::Sym("stage_back"), elmcode, 1);
    case 38:  // FRONT
      return elm::MakeChain(Type::Stage, elm::Sym("stage_front"), elmcode, 1);
    case 73:  // NEXT
      return elm::MakeChain(Type::Stage, elm::Sym("stage_next"), elmcode, 1);

    case 65:  // EXCALL
      return elm::MakeChain(Type::Excall, elm::Sym("excall"), elmcode, 1);

    case 135:  // MASK
      return elm::MakeChain(Type::MaskList, elm::Sym("mask"), elmcode, 1);

    case 63:  // SYSCOM
      return elm::MakeChain(Type::Syscom, elm::Sym("syscom"), elmcode, 1);
    case 64:  // SYSTEM
      return elm::MakeChain(Type::System, elm::Sym("system"), elmcode, 1);

    case 54:    // WAIT
    case 55: {  // WAIT_KEY
      Wait wait;
      wait.interruptable = root == 55;
      wait.time_ms = AsInt(bind.arglist[0]);
      return AccessChain{.root = std::move(wait)};
    }

    case 92:  // SYSTEM
      return elm::MakeChain(Type::System, elm::Sym("os"), elmcode, 1);

    case 40:  // COUNTER
      return elm::MakeChain(Type::CounterList, elm::Sym("counter"), elmcode, 1);

    case 79:  // FRAME_ACTION
      return elm::MakeChain(Type::FrameAction, elm::Sym("frame_action"),
                            elmcode, 1);
    case 53:  // FRAME_ACTION_CH
      return elm::MakeChain(Type::FrameActionList, elm::Sym("frame_action_ch"),
                            elmcode, 1);

    default: {
      elm::AccessChain uke;
      uke.root = elm::Sym('<' + ToString(elmcode.code.front()) + '>');
      uke.nodes.reserve(elmcode.code.size() - 1);
      for (size_t i = 1; i < elmcode.code.size(); ++i)
        uke.nodes.emplace_back(elm::Val(elmcode.code[i]));
      return uke;
    }
  }
}

AccessChain MakeChain(Root root, std::span<const Value> elmcode, BindCtx bind) {
  AccessChain result{.root = std::move(root), .nodes = {}};
  result.nodes.reserve(elmcode.size());

  flat_map<Builder> const* mp;
  while ((mp = elm::GetMethodMap(result.GetType())) != nullptr) {
    if (!mp || elmcode.empty())
      break;
    if (!std::holds_alternative<Integer>(elmcode.front()))
      break;
    const int key = AsInt(elmcode.front());
    if (!mp->contains(key))
      break;

    Builder::Ctx ctx{.elmcode = elmcode, .chain = result};
    mp->at(key).Build(ctx);
  }

  const auto cur_type = result.GetType();
  for (const auto& it : elmcode)
    result.nodes.emplace_back(elm::Node(cur_type, elm::Val(it)));

  return result;
}

AccessChain MakeChain(Type root_type,
                      Root::var_t root_node,
                      ElementCode const& elmcode,
                      size_t subidx,
                      BindCtx bind) {
  return MakeChain(Root(root_type, std::move(root_node)),
                   std::span{elmcode.code}.subspan(subidx), std::move(bind));
}

AccessChain Bind(AccessChain chain,
                 int overload,
                 std::vector<Value> arg,
                 std::vector<std::pair<int, Value>> named_arg,
                 std::optional<Type> ret_type) {
  Callable* callable = std::get_if<Callable>(&chain.nodes.back().var);
  if (callable == nullptr)
    throw std::runtime_error("Bind: Expected callable, but got " +
                             chain.ToDebugString());

  auto candidate = std::find_if(
      callable->overloads.begin(), callable->overloads.end(),
      [overload](const Function& fn) {
        return !fn.overload.has_value() || *fn.overload == overload;
      });
  if (candidate == callable->overloads.end())
    throw std::runtime_error("Bind: Overload " + std::to_string(overload) +
                             " not found in " + chain.ToDebugString());

  // TODO: Implement binding logic

  if (ret_type.has_value() && *ret_type != chain.GetType()) {
    static DomainLogger logger("Bind");
    auto rec = logger(Severity::Warn);
    rec << "return type mismatch: " << ToString(*ret_type) << " vs "
        << ToString(chain.GetType()) << '\n';
    rec << chain.ToDebugString();
  }

  return chain;
}

}  // namespace libsiglus::elm
