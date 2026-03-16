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
#include <limits>
#include <stdexcept>
#include <variant>

#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"
#include "utilities/assertx.hpp"
#include "utilities/overload.hpp"
#include "vm/instruction.hpp"
#include "vm/string.hpp"

namespace sr = serilang;

namespace libsiglus {

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
    : gc_(std::move(gc)) {
  chunk_ = gc_->Allocate<sr::Code>();
  chunk_->const_pool.emplace_back(chunk_);
}
Recompiler::~Recompiler() = default;

void Recompiler::Gen(token::Token_t tok) {
  try {
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

void Recompiler::emit_current_chunk() { emit(sr::Push{0}); }

uint32_t Recompiler::constant(sr::Value v) {
  if (!chunk_)
    throw std::logic_error("Recompiler chunk is not set");

  const auto next_slot = static_cast<uint32_t>(chunk_->const_pool.size());
  auto [it, ok] = const_pool_.emplace(std::move(v), next_slot);
  if (ok)
    chunk_->const_pool.emplace_back(it->first);
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
  reserve_fast_local(chunk_, id);
  emit(sr::StoreFast{fast_local_slot(id)});
}
void Recompiler::emit_load_fast(int id) {
  reserve_fast_local(chunk_, id);
  emit(sr::LoadFast{fast_local_slot(id)});
}
void Recompiler::emit_store_global(std::string id) {
  emit(sr::StoreGlobal{intern_name(std::move(id))});
}
void Recompiler::emit_load_global(std::string id) {
  emit(sr::LoadGlobal{intern_name(std::move(id))});
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
  switch (static_cast<sr::OpCode>((*chunk_)[site])) {
    case sr::OpCode::Jump:
    case sr::OpCode::JumpIfFalse:
    case sr::OpCode::JumpIfTrue:
    case sr::OpCode::TryBegin: {
      const auto offset =
          rel<int32_t>(site + sizeof(std::byte) + sizeof(int32_t), target);
      chunk_->Write(site + 1, offset);
    } break;

    case sr::OpCode::MakeFunction:
      chunk_->Write(site + 1, static_cast<uint32_t>(target));
      break;

    default:
      throw std::runtime_error(
          "Codegen: invalid patch site (type" +
          std::to_string(static_cast<uint8_t>((*chunk_)[site])) + ')');
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
  emit_elm(tk.chain);                          // (elm)
  emit(sr::GetField{intern_name("get")});      // (getter)
  emit(sr::Call{.argcnt = 0, .kwargcnt = 0});  // (val)
  emit_store_fast(tk.dst.id);                  // -> ()
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
  emit_elm(tk.dst);                        // (proxy)
  emit(sr::GetField{intern_name("set")});  // (proxy.setter)
  emit_val(tk.src);
  emit(sr::Call({.argcnt = 1, .kwargcnt = 0}));
  // (proxy.setter, value) -> ()
}
void Recompiler::emit_tok(const token::Duplicate& tk) {
  emit_val(tk.src);
  emit_store_fast(tk.dst.id);
}
void Recompiler::emit_tok(const token::Subroutine& tk) {
  auto loc = code_size();
  auto [it, ok] = subroutine_entries_.emplace(tk.name, loc);
  if (!ok)
    throw std::runtime_error("redefinition of subroutine " + tk.name);
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
void Recompiler::emit_tok(const token::Eof& tk) {
  // nop
}

// element codegen
void Recompiler::emit_elm(const elm::AccessChain& e) {
  bool is_first = true;
  if (!std::holds_alternative<std::monostate>(e.root.var)) {
    is_first = false;
    std::visit([&](const auto& rt) { emit_elm_root(rt); }, e.root.var);
  }

  if (e.nodes.empty())
    return;

  for (size_t i = 0; i < e.nodes.size(); ++i) {
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
  // (result)
}

void Recompiler::emit_elm_root(const std::monostate& r) {
  ASSERTX_TRUE(false);  // unreachable
}
void Recompiler::emit_elm_root(const elm::Usrcmd& r) {
  emit_load_global("__builtin_usrcmd");
  emit_const(r.scene), emit_const(r.entry), emit_const(std::string(r.name));
  emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
  // (usrcmd)

  const auto& arg = r.arguments;
  emit_const(arg.overload_id);
  // (usrcmd, ol)

  for (auto const& it : arg.arg)
    emit_val(it);
  emit(sr::MakeList{.nelms = arg.arg.size()});
  // (usrcmd, ol, list)

  for (auto const& [k, it] : arg.named_arg)
    emit_const(k), emit_val(it);
  emit(sr::MakeDict{.nelms = arg.named_arg.size()});
  // (usrcmd, ol, list, dict)

  emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
  // (ret)
}
void Recompiler::emit_elm_root(const elm::Usrprop& r) {
  emit_load_global("__builtin_usrprop");
  emit_const(r.scene);
  emit_const(r.idx);
  emit_const(std::string(r.name));
  emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
  // (usrprop, scene, idx, name) -> (prop)
}
void Recompiler::emit_elm_root(const elm::Arg& r) {
  // fast: (arg_0, arg_1, ...)
  emit_load_fast(r.id);
  // (arg)
}
void Recompiler::emit_elm_root(const elm::Farcall& r) {
  emit_load_global("__builtin_farcall");
  emit_val(r.scn_name), emit_val(r.zlabel);
  emit(sr::Call{.argcnt = 2, .kwargcnt = 0});
  // (farcall, scn, z) -> (fn)
  for (auto const& it : r.intargs)
    emit_val(it);
  emit(sr::MakeList{.nelms = r.intargs.size()});
  for (auto const& it : r.strargs)
    emit_val(it);
  emit(sr::MakeList{.nelms = r.strargs.size()});
  emit(sr::Call{.argcnt = 2, .kwargcnt = 0});
  // (fn, intargs, strargs) -> (ret)
}

void Recompiler::emit_elm_node(const elm::Member& nd) {
  // (primary)
  uint32_t slot = intern_name(std::string(nd.name));
  emit(sr::GetField{slot});
  // (member)
}
void Recompiler::emit_elm_node(const elm::Call& nd) {
  // (fn)
  if (nd.overload_id)
    emit_const(*nd.overload_id);
  else
    emit_const_nil();
  // (fn, ol)

  for (auto const& arg : nd.args)
    emit_val(arg);
  emit(sr::MakeList{.nelms = nd.args.size()});
  // (fn, al, list)

  for (auto const& [k, arg] : nd.kwargs)
    emit_const(k), emit_val(arg);
  emit(sr::MakeDict{.nelms = nd.kwargs.size()});
  // (fn, al, list, dist)

  emit(sr::Call{.argcnt = 3, .kwargcnt = 0});
  // (ret)
}
void Recompiler::emit_elm_node(const elm::Subscript& nd) {
  // (primary)
  emit_val(nd.idx);     // (primary, idx)
  emit(sr::GetItem{});  // (item)
}

}  // namespace libsiglus
