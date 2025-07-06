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

#include "libsiglus/element_builder.hpp"

namespace libsiglus::elm {

ElementParser::ElementParser(std::unique_ptr<Context> ctx)
    : ctx_(std::move(ctx)) {}
ElementParser::~ElementParser() = default;

AccessChain ElementParser::Parse(ElementCode& elm) {
  const int flag = elm.At<int>(0);
  const size_t idx = flag ^ (flag << 24);

  if (flag == USER_COMMAND_FLAG)
    return resolve_usrcmd(elm, idx);
  else if (flag == USER_PROPERTY_FLAG)
    return resolve_usrprop(elm, idx);

  return resolve_element(elm);
}

AccessChain ElementParser::resolve_usrcmd(ElementCode& elm, size_t idx) {
  AccessChain chain;

  const libsiglus::Command* cmd = nullptr;
  if (idx < ctx_->GlobalCommands().size())
    cmd = &ctx_->GlobalCommands()[idx];
  else
    cmd = &ctx_->SceneCommands()[idx - ctx_->GlobalCommands().size()];

  chain.root = elm::Usrcmd{
      .scene = cmd->scene_id, .entry = cmd->offset, .name = cmd->name};
  // return type?
  return chain;
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

  return elm::MakeChain(root_type, std::move(root), elm, 1);
}

AccessChain ElementParser::resolve_element(ElementCode& elmcode) {
  auto elm = elmcode.IntegerView();
  int root = elm.front();

  switch (root) {
    case 83: {  // CUR_CALL
      const int elmcall = elm[1];
      if ((elmcall >> 24) == 0x7d) {
        auto id = (elmcall ^ (0x7d << 24));
        return elm::MakeChain(ctx_->CurcallArgs()[id], elm::Arg(id), elmcode,
                              2);
      }

      else if (elmcall == 0)
        return elm::MakeChain(Type::IntList, elm::Sym("L"), elmcode, 2);

      else if (elmcall == 1)
        return elm::MakeChain(Type::StrList, elm::Sym("K"), elmcode, 2);
    } break;

      // ====== KOE(Sound) ======
      // needs kidoku flag
    case 18:  // KOE
    case 90:  // KOE_PLAY_WAIT
    case 91:  // KOE_PLAY_WAIT_KEY
      ctx_->ReadKidoku();
      break;

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

    case 12:  // MWND_PRINT
      ctx_->ReadKidoku();
      break;

    [[likely]]
    default:
      break;
  }

  return elm::MakeChain(elmcode);
}

}  // namespace libsiglus::elm
