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

#pragma once

#include "libsiglus/element.hpp"
#include "libsiglus/token.hpp"
#include "vm/gc.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>

namespace libsiglus {

struct CompileError {
  std::string message;
  std::optional<token::Token_t> token;
};

class Recompiler {
 public:
  explicit Recompiler(std::shared_ptr<serilang::GarbageCollector> gc);
  ~Recompiler();

  inline bool Ok() const { return errors_.empty(); }
  inline std::span<const CompileError> GetErrors() const { return errors_; }
  inline void ClearErrors() { errors_.clear(); }

  void Gen(token::Token_t tok);

 private:
  // Data members
  std::shared_ptr<serilang::GarbageCollector> gc_;

 public:
  std::unordered_map<std::string, std::size_t> subroutine_entries_;
  serilang::Code* chunk_ = nullptr;

 private:
  // Error handling
  std::vector<CompileError> errors_;
  inline void AddError(std::string msg,
                       std::optional<token::Token_t> tok = std::nullopt) {
    errors_.emplace_back(std::move(msg), std::move(tok));
  }

  // Emit helpers
  template <typename T>
  inline void emit(T ins) {
    chunk_->Append(std::move(ins));
  }
  inline std::size_t code_size() const { return chunk_->code.size(); }
  void emit_current_chunk();

  // Constant pool
  std::unordered_map<serilang::Value, uint32_t> const_pool_;
  uint32_t constant(serilang::Value v);
  uint32_t emit_const_nil();
  uint32_t emit_const(int v);
  uint32_t emit_const(std::string v);
  uint32_t intern_name(std::string v);

  // Fast locals
  int fast_local_cnt_ = 0;
  void emit_store_fast(int id);
  void emit_load_fast(int id);

  void emit_store_global(std::string id);
  void emit_load_global(std::string id);

  // Patch sites and labels
  std::vector<std::optional<std::size_t>> label_offsets_;
  std::vector<std::vector<std::size_t>> patch_sites_;
  void add_patch_site(int lid, std::size_t site);
  void patch(std::size_t site, std::size_t target);

 private:
  void emit_val(const Value& v);

  // Token codegen
  void emit_tok(const token::ElmAlias& tk);
  void emit_tok(const token::Command& tk);
  void emit_tok(const token::Name& tk);
  void emit_tok(const token::Textout& tk);
  void emit_tok(const token::GetProperty& tk);
  void emit_tok(const token::Operate1& tk);
  void emit_tok(const token::Operate2& tk);
  void emit_tok(const token::Label& tk);
  void emit_tok(const token::Goto& tk);
  void emit_tok(const token::GotoIf& tk);
  void emit_tok(const token::Gosub& tk);
  void emit_tok(const token::Assign& tk);
  void emit_tok(const token::Duplicate& tk);
  void emit_tok(const token::Subroutine& tk);
  void emit_tok(const token::Return& tk);
  void emit_tok(const token::Eof& tk);

  // Element/AccessChain codegen
  void emit_elm(const elm::AccessChain& e);
  void emit_elm_root(const std::monostate& r);
  void emit_elm_root(const elm::Usrcmd& r);
  void emit_elm_root(const elm::Usrprop& r);
  void emit_elm_root(const elm::Arg& r);
  void emit_elm_root(const elm::Farcall& r);
  void emit_elm_node(const elm::Member& nd);
  void emit_elm_node(const elm::Call& nd);
  void emit_elm_node(const elm::Subscript& nd);
};

}  // namespace libsiglus
