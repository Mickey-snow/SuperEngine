// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "libsiglus/recompiler.hpp"

#include <exception>
#include <format>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "libsiglus/element.hpp"
#include "libsiglus/intern_name.hpp"
#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"
#include "log/core.hpp"
#include "log/domain_logger.hpp"
#include "utilities/assertx.hpp"
#include "utilities/overload.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"

namespace sr = serilang;

namespace libsiglus {

DomainLogger logger("Recompiler");

std::string CompileError::ToString() const {
  std::string ret;
  if (token)
    ret = ::libsiglus::ToString(*token) + '\n';
  ret += message;
  return ret;
}

namespace {

uint16_t fast_local_slot(int id) {
  if (id < 0)
    throw std::runtime_error("Codegen: invalid fast-local id " +
                             std::to_string(id));
  if (id > std::numeric_limits<uint16_t>::max())
    throw std::runtime_error("Codegen: too many fast locals (" +
                             std::to_string(id) + ')');
  return static_cast<uint16_t>(id);
}

void reserve_fast_local(sr::Code* chunk, int id) {
  const auto slot = static_cast<std::size_t>(fast_local_slot(id));
  for (std::size_t i = chunk->fast_locals.size(); i <= slot; ++i)
    chunk->fast_locals.emplace_back("v" + std::to_string(i));
}

}  // namespace

Recompiler::Recompiler(std::shared_ptr<serilang::GarbageCollector> gc)
    : gc_(gc) {
  cur_chunk_ = gc_->Allocate<sr::Code>();
  module_ = gc_->Allocate<sr::Module>("<siglus script>");
  cur_chunk_->const_pool.emplace_back(cur_chunk_);

  initial_jump_site_ = code_size();
  emit(sr::Jump{0});
  main_entry_ = code_size();
}
Recompiler::~Recompiler() = default;

void Recompiler::SetSceneProperties(int scene_id,
                                    std::vector<Property> properties) {
  scene_id_ = scene_id;
  scene_properties_ = std::move(properties);
}

void Recompiler::Gen(token::Token_t tok) {
  try {
    if (is_finalized_)
      throw std::runtime_error("cannot emit token after EOF");

    if (is_debug_) {
      emit_const(ToString(tok));
      emit(sr::Dup{});
      emit_load_global("__builtin_dbgprint");
      emit(sr::Swap{});
      emit(sr::Call{.argcnt = 1, .kwargcnt = 0});
      emit(sr::Pop{});
      emit(sr::DebugValue{});
    }
    std::visit([this](const auto& stmt) { emit_tok(stmt); }, std::move(tok));
  } catch (std::exception& e) {
    AddError(e.what(), tok);
  }
}

void Recompiler::Finish() {
  if (is_finalized_)
    return;

  // Payload functions should not fall through into the bootstrap block.
  emit_const_nil();
  emit(sr::Return{});

  const std::size_t bootstrap_entry = code_size();
  patch(initial_jump_site_, bootstrap_entry);
  emit_store_function_global("%%script", main_entry_, 0);

  for (const auto& record : subroutines_) {
    const std::string entry_name = GetUsercmdId(record.source_entry);
    emit_store_function_global(entry_name, record.bytecode_entry,
                               record.args.size());
  }

  for (const auto& record : zlabels_) {
    const std::string entry_name = GetZlabelId(record.id);
    emit_store_function_global(entry_name, record.bytecode_entry, 0);
  }

  if (scene_id_.has_value()) {
    emit_scene_property_table();
    emit_store_global("__usrprop");  // __usrprop -> list
  }

  emit_const_nil();
  emit(sr::Return{});

  is_finalized_ = true;
}

void Recompiler::emit_current_chunk() { emit(sr::Push{0}); }

uint32_t Recompiler::constant(sr::Value v) {
  if (!cur_chunk_)
    throw std::logic_error("Recompiler chunk is not set");

  const auto next_slot = static_cast<uint32_t>(cur_chunk_->const_pool.size());
  auto [it, ok] = const_pool_.emplace(std::move(v), next_slot);
  if (ok)
    cur_chunk_->const_pool.emplace_back(it->first);
  return it->second;
}
uint32_t Recompiler::emit_const_nil() {
  uint32_t slot = constant(sr::nil);
  emit(sr::Push{slot});
  return slot;
}
uint32_t Recompiler::emit_const(int v) {
  uint32_t slot = constant(sr::Value(v));
  emit(sr::Push{slot});
  return slot;
}
uint32_t Recompiler::emit_const(std::string v) {
  uint32_t slot = intern_name(std::move(v));
  emit(sr::Push{slot});
  return slot;
}
uint32_t Recompiler::intern_name(std::string v) {
  sr::String* str = gc_->Allocate<sr::String>(std::move(v));
  uint32_t slot = constant(sr::Value(str));
  return slot;
}

void Recompiler::emit_store_fast(int id) {
  reserve_fast_local(cur_chunk_, id);
  emit(sr::StoreFast{fast_local_slot(id)});
}
void Recompiler::emit_load_fast(int id) {
  reserve_fast_local(cur_chunk_, id);
  emit(sr::LoadFast{fast_local_slot(id)});
}
void Recompiler::emit_store_global(std::string id) {
  emit(sr::StoreGlobal{intern_name(std::move(id))});
}
void Recompiler::emit_load_global(std::string id) {
  emit(sr::LoadGlobal{intern_name(std::move(id))});
}

void Recompiler::emit_make_function(std::size_t entry, std::size_t nargs) {
  if (nargs > 0)
    reserve_fast_local(cur_chunk_, static_cast<int>(nargs));

  emit_current_chunk();
  for (std::size_t i = 0; i < nargs; ++i)
    emit_const("arg" + std::to_string(i));

  emit(sr::MakeFunction{.entry = static_cast<uint32_t>(entry),
                        .nparam = static_cast<uint32_t>(nargs),
                        .ndefault = 0,
                        .has_vararg = false,
                        .has_kwarg = false});
}

void Recompiler::emit_store_function_global(std::string id,
                                            std::size_t entry,
                                            std::size_t nargs) {
  emit_make_function(entry, nargs);
  emit_store_global(std::move(id));
}

void Recompiler::emit_scene_property_table() {
  for (const Property& property : scene_properties_) {
    switch (property.form) {
      case Type::Int:
        emit_const(0);
        break;
      case Type::IntList:
        emit_load_global("make_intlist");
        emit_const(property.size);
        emit(sr::Call{.argcnt = 1, .kwargcnt = 0});
        break;
      case Type::String:
        emit_const("");
        break;
      case Type::StrList:
        emit_load_global("make_strlist");
        emit_const(property.size);
        emit(sr::Call{.argcnt = 1, .kwargcnt = 0});
        break;
      default:
        emit_const_nil();
        break;
    }
  }

  emit(sr::MakeList{.nelms = static_cast<uint32_t>(scene_properties_.size())});
}

void Recompiler::emit_load_proplist(int scene) {
  if (scene == -1) {
    emit_load_global("__globalprop");
    return;
  }

  if (scene == scene_id_) {
    emit_load_global("__usrprop");
    return;
  }

  emit_load_global("__builtin_load_scn");
  emit_const(scene);
  emit(sr::Call{.argcnt = 1, .kwargcnt = 0});
  emit(sr::GetField{intern_name("__usrprop")});
}

void Recompiler::add_patch_site(int lid, std::size_t site) {
  if (lid >= patch_sites_.size()) {
    patch_sites_.resize(lid + 1), label_offsets_.resize(lid + 1);
  }

  if (label_offsets_[lid].has_value())
    patch(site, *label_offsets_[lid]);
  else
    patch_sites_[lid].emplace_back(site);
}

template <typename offset_t>
static inline constexpr auto rel(offset_t from, offset_t to) -> offset_t {
  return to - from;
}
void Recompiler::patch(std::size_t site,
                       std::size_t target) {  // Patch jumps
  switch (static_cast<sr::OpCode>((*cur_chunk_)[site])) {
    case sr::OpCode::Jump:
    case sr::OpCode::JumpIfFalse:
    case sr::OpCode::JumpIfTrue:
    case sr::OpCode::TryBegin: {
      const auto offset =
          rel<int32_t>(site + sizeof(std::byte) + sizeof(int32_t), target);
      cur_chunk_->Write(site + 1, offset);
    } break;

    case sr::OpCode::MakeFunction:
      cur_chunk_->Write(site + 1, static_cast<uint32_t>(target));
      break;

    default:
      throw std::runtime_error(
          "Codegen: invalid patch site (type" +
          std::to_string(static_cast<uint8_t>((*cur_chunk_)[site])) + ')');
  }
}

// -----------------------------------------------------------------------
// token codegen
void Recompiler::emit_val(const Value& v) {
  std::visit(
      overload([&](Integer const& v) { emit_const(v.val_); },
               [&](String const& v) { emit_const(v.val_); },
               [&](Variable const& v) { emit_load_fast(v.id); },
               [&](const auto&) {
                 throw std::runtime_error("Cannot emit value " + ToString(v));
               }),
      v);
}

void Recompiler::emit_tok(const token::ElmAlias& tk) {
  emit_elm(tk.chain);
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Command& tk) {
  emit_elm(tk.chain);
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Name& tk) {
  emit_load_global("__builtin_name");
  emit_val(tk.str);
  emit(sr::Call{.argcnt = 1});
}
void Recompiler::emit_tok(const token::Textout& tk) {
  emit_load_global("__builtin_textout");
  emit_const(tk.kidoku), emit_val(tk.str);
  emit(sr::Call{.argcnt = 2});
}
void Recompiler::emit_tok(const token::GetProperty& tk) {
  emit_elm(tk.chain);
  emit_store_fast(tk.dst.id);  // (val) -> ()
}
void Recompiler::emit_tok(const token::Operate1& tk) {
  if (tk.val) {
    emit_val(*tk.val);
    return;
  }
  emit_val(tk.rhs);
  emit(sr::UnaryOp{LowerUnaryOperator(tk.op)});
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Operate2& tk) {
  if (tk.val) {
    emit_val(*tk.val);
    return;
  }
  emit_val(tk.lhs), emit_val(tk.rhs);
  emit(sr::BinaryOp{LowerBinaryOperator(tk.op)});
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Label& tk) {
  const int lid = tk.id;
  if (label_offsets_.size() <= lid) {
    label_offsets_.resize(lid + 1), patch_sites_.resize(lid + 1);
  }
  const std::size_t target = code_size();

  label_offsets_[lid] = target;
  for (auto site : patch_sites_[lid])
    patch(site, target);
  patch_sites_[lid].clear();
}
void Recompiler::emit_tok(const token::Zlabel& tk) {
  const std::size_t loc = code_size();
  auto [it, ok] = zlabel_entries_.emplace(tk.id, loc);
  if (!ok) {
    throw std::runtime_error(
        std::format("redefinition of zlabel entry {}", tk.id));
  }

  zlabels_.push_back(ZlabelRecord{.id = tk.id, .bytecode_entry = loc});
}
void Recompiler::emit_tok(const token::Goto& tk) {
  auto site = code_size();
  emit(sr::Jump{0});
  add_patch_site(tk.label, site);
}
void Recompiler::emit_tok(const token::GotoIf& tk) {
  emit_val(tk.src);
  auto site = code_size();
  if (tk.cond)
    emit(sr::JumpIfTrue{0});
  else
    emit(sr::JumpIfFalse{0});
  add_patch_site(tk.label, site);
}
void Recompiler::emit_tok(const token::Gosub& tk) {
  emit_current_chunk();
  for (int i = 0; i < tk.args.size(); ++i)
    emit_const("arg" + std::to_string(i));

  auto site = code_size();
  emit(sr::MakeFunction{.entry = 0,
                        .nparam = tk.args.size(),
                        .ndefault = 0,
                        .has_vararg = false,
                        .has_kwarg = false});
  add_patch_site(tk.entry_id, site);

  for (auto const& arg : tk.args)
    emit_val(arg);
  emit(sr::Call{.argcnt = tk.args.size(), .kwargcnt = 0});
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Assign& tk) {
  emit_elm(tk.dst, &tk.src);
}
void Recompiler::emit_tok(const token::Duplicate& tk) {
  emit_val(tk.src);
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Subroutine& tk) {
  const std::size_t loc = code_size();
  auto [it, ok] = subroutine_entries_.emplace(tk.source_entry, loc);
  if (!ok) {
    throw std::runtime_error(
        std::format("redefinition of user command entry {}", tk.source_entry));
  }

  subroutines_.push_back(SubroutineRecord{.name = tk.name,
                                          .source_entry = tk.source_entry,
                                          .bytecode_entry = loc,
                                          .args = tk.args});
}
void Recompiler::emit_tok(const token::Return& tk) {
  if (tk.ret_vals.size() == 0)
    emit_const_nil();
  else if (tk.ret_vals.size() == 1)
    emit_val(tk.ret_vals.front());
  else {
    for (const auto& it : tk.ret_vals)
      emit_val(it);
    emit(sr::MakeList{.nelms = tk.ret_vals.size()});
  }
  emit(sr::Return{});
}
void Recompiler::emit_tok(const token::Eof& tk) { Finish(); }

// element codegen
void Recompiler::emit_elm(const elm::AccessChain& e, const Value* assign) {
  if (auto* prop = std::get_if<elm::Usrprop>(&e.root.var);
      prop && assign && e.nodes.empty()) {
    emit_load_proplist(prop->scene);  // (prop[])

    emit_const(prop->idx), emit_val(*assign);
    emit(sr::SetItem{});  // (prop[], idx, val) -> ()
    return;
  }

  bool is_first = true;
  if (!std::holds_alternative<std::monostate>(e.root.var)) {
    is_first = false;
    std::visit([&](const auto& rt) { emit_elm_root(rt); }, e.root.var);
  }

  auto fail = [] { throw std::runtime_error("cannot assign to this element"); };
  if (e.nodes.empty()) {
    if (assign)
      fail();
    return;
  }

  size_t n = e.nodes.size();
  if (assign)
    --n;
  if (assign && std::holds_alternative<elm::Call>(e.nodes.back().var)) {
    ASSERTX_GE(n, 1);
    --n;
  }

  for (size_t i = 0; i < n; ++i) {
    if (i == 0 && is_first) {
      if (auto* sym = std::get_if<elm::Member>(&e.nodes.front().var)) {
        emit_load_global(std::string(sym->name));
        continue;
      } else {
        throw std::runtime_error("[Recompiler] cannot compile element: " +
                                 e.ToDebugString());
      }
    }
    std::visit([&](const auto& nd) { emit_elm_node(nd); }, e.nodes[i].var);
  }

  if (assign) {
    if (auto* mem = std::get_if<elm::Member>(&e.nodes.back().var)) {
      emit_val(*assign);
      auto name_slot = intern_name(std::string(mem->name));
      emit(sr::SetField{name_slot});
    } else if (auto* itm = std::get_if<elm::Subscript>(&e.nodes.back().var)) {
      emit_val(itm->idx), emit_val(*assign);
      emit(sr::SetItem{});
    } else if (auto* call = std::get_if<elm::Call>(&e.nodes.back().var)) {
      auto* mem = std::get_if<elm::Member>(&e.nodes[e.nodes.size() - 2].var);
      ASSERTX_NE(mem, nullptr);
      std::string fn(mem->name);
      fn = "set_" + fn;
      emit(sr::GetField{intern_name(std::move(fn))});  // (setter)
      ASSERTX_TRUE(call->args.empty() && call->kwargs.empty());
      emit_val(*assign);
      emit(sr::Call{.argcnt = 1, .kwargcnt = 0});  // (setter, val) -> (nil)
      emit(sr::Pop{});
    } else
      fail();
  }
}

void Recompiler::emit_elm_root(const std::monostate& r) {
  ASSERTX_TRUE(false);  // unreachable
}
void Recompiler::emit_elm_root(const elm::Usrcmd& r) {
  const std::string cmd_name = GetUsercmdName(std::string(r.name));

  emit_load_global("__builtin_usrcmd");
  emit_const(r.scene), emit_const(r.entry), emit_const(std::string(r.name));
  emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
  // (usrcmd)

  const auto& arg = r.arguments;
  if (arg.overload_id != 0) {
    logger(Severity::Error)
        << "Usrcmd " << cmd_name << " has overload: " << arg.overload_id;
  }

  for (auto const& it : arg.arg)
    emit_val(it);

  if (!arg.named_arg.empty()) {
    emit_const("unsupported named/tagged user-command arguments for " +
               cmd_name);
    emit(sr::Throw{});
    return;
  }

  emit(sr::Call{.argcnt = arg.arg.size(), .kwargcnt = 0});
  // (usrcmd, args...) -> (ret)
}
void Recompiler::emit_elm_root(const elm::Usrprop& r) {
  emit_load_proplist(r.scene);
  // (prop[])
  emit_const(r.idx);
  emit(sr::GetItem{});  // (prop[], idx) -> (item)
}
void Recompiler::emit_elm_root(const elm::Arg& r) {
  // fast: (arg_0, arg_1, ...)
  emit_load_fast(r.id + 1);
  // (arg)
}
void Recompiler::emit_elm_root(const elm::Farcall& r) {
  emit_load_global("__builtin_farcall");
  emit_val(r.scn_name), emit_val(r.zlabel);
  emit(sr::Call{.argcnt = 2, .kwargcnt = 0});
  // (farcall, scn, z) -> (fn)

  emit_load_global("__builtin_push_frame");
  for (auto const& it : r.intargs)
    emit_val(it);
  emit(sr::MakeList{.nelms = r.intargs.size()});
  for (auto const& it : r.strargs)
    emit_val(it);
  emit(sr::MakeList{.nelms = r.strargs.size()});
  emit(sr::Call{.argcnt = 2, .kwargcnt = 0});
  // (fn, push_frame, L[], K[]) -> (fn, nil)
  emit(sr::Pop{});
  // (fn, nil) -> (fn)

  emit(sr::Call{.argcnt = 0, .kwargcnt = 0});
  // -> (ret)

  emit_load_global("__builtin_pop_frame");
  emit(sr::Call{.argcnt = 0, .kwargcnt = 0});
  emit(sr::Pop{});
  // (ret,nil) -> (ret)
}

void Recompiler::emit_elm_node(const elm::Member& nd) {
  // (primary)
  uint32_t slot = intern_name(std::string(nd.name));
  emit(sr::GetField{slot});
  // (member)
}
void Recompiler::emit_elm_node(const elm::Call& nd) {
  // (fn)
  if (nd.is_simple) {
    if (!nd.kwargs.empty()) {
      std::string msg = std::format(
          "Simple Siglus callable has {} tagged/keyword argument(s); ignoring "
          "kwargs",
          nd.kwargs.size());
      AddError(msg);
      logger(Severity::Warn) << msg;
    }

    // no overload, no kwargs
    for (auto const& arg : nd.args)
      emit_val(arg);
    // (fn, args...)
    emit(sr::Call(
        {.argcnt = static_cast<uint32_t>(nd.args.size()), .kwargcnt = 0}));
    // -> (ret)
  } else {
    if (nd.overload_id)
      emit_const(*nd.overload_id);
    else
      emit_const_nil();
    // (fn, ol)

    for (auto const& arg : nd.args)
      emit_val(arg);
    emit(sr::MakeList{.nelms = nd.args.size()});
    // (fn, ol, args)

    for (auto const& [k, arg] : nd.kwargs) {
      emit_const("_" + std::to_string(k));
      emit_val(arg);
    }
    emit(sr::MakeDict{.nelms = nd.kwargs.size()});
    // (fn, ol, args, kwargs)

    emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
    // -> (ret)
  }
}
void Recompiler::emit_elm_node(const elm::Subscript& nd) {
  // (primary)
  emit_val(nd.idx);     // (primary, idx)
  emit(sr::GetItem{});  // (item)
}

}  // namespace libsiglus
