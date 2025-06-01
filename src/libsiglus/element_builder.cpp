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

#include "utilities/flat_map.hpp"

namespace libsiglus::elm {

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
static flat_map<Builder> const* GetMethodMap(Type type) {
  switch (type) {
    case Type::IntList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(Type::Int, Subscript{ctx.elmcode[1]});
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
          id[3] | b(Type::IntList, Member("b1")),
          id[4] | b(Type::IntList, Member("b2")),
          id[5] | b(Type::IntList, Member("b4")),
          id[7] | b(Type::IntList, Member("b8")),
          id[6] | b(Type::IntList, Member("b16")),
          id[10] | b(Type::Callable, Member("init")),
          id[2] | b(Type::Callable, Member("resize")),
          id[9] | b(Type::Callable, Member("size")),
          id[8] | b(Type::Callable, Member("fill")),
          id[1] | b(Type::Callable, Member("Set")));
      return &mp;
    }

    case Type::StrList: {
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(Type::String,
                                         Call("substr", {ctx.elmcode[1]}));
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
          id[3] | b(Type::Callable, Member("init")),
          id[2] | b(Type::Callable, Member("resize")),
          id[4] | b(Type::Callable, Member("size")));
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
      static const auto mp = make_flatmap<Builder>(
          id[-1] | Builder([](Builder::Ctx& ctx) {
            ctx.chain.nodes.emplace_back(Type::FrameAction,
                                         Subscript{ctx.elmcode[1]});
            ctx.elmcode = ctx.elmcode.subspan(2);
          }),
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
          make_flatmap<Builder>(id[-1] | Builder([](Builder::Ctx& ctx) {
                                  ctx.chain.nodes.emplace_back(
                                      Type::Counter, Subscript{ctx.elmcode[1]});
                                  ctx.elmcode = ctx.elmcode.subspan(2);
                                }),
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
    }

    default:
      return nullptr;
  }
}

// -----------------------------------------------------------------------
AccessChain make_chain(Root root, std::span<const Value> elmcode) {
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
AccessChain make_chain(Type root_type,
                       Root::var_t root_node,
                       ElementCode const& elmcode,
                       size_t subidx) {
  return make_chain(Root(root_type, std::move(root_node)),
                    std::span{elmcode.code}.subspan(subidx));
}

}  // namespace libsiglus::elm
