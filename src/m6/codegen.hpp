// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include "m6/ast.hpp"
#include "m6/exception.hpp"
#include "vm/chunk.hpp"
#include "vm/instruction.hpp"
#include "vm/value.hpp"

#include <concepts>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace m6 {

class CodeGenerator {
 public:
  explicit CodeGenerator(bool repl = false);
  ~CodeGenerator();

  bool Ok() const;
  std::span<const Error> GetErrors() const;
  void ClearErrors();

  std::shared_ptr<serilang::Chunk> GetChunk() const;
  void SetChunk(std::shared_ptr<serilang::Chunk> c);

  void Gen(std::shared_ptr<AST> ast);

 private:
  // -- Type aliases ----------------------------------------------------
  using Value = serilang::Value;
  using ChunkPtr = std::shared_ptr<serilang::Chunk>;
  using Scope = std::unordered_map<std::string, std::size_t>;

  // -- Data members ---------------------------------------------------
  bool repl_mode_;
  ChunkPtr chunk_;
  std::vector<Scope> locals_;
  std::size_t local_depth_;
  std::unordered_map<std::string, int32_t> patch_sites_;
  std::vector<Error> errors_;

  // -- Error handling -------------------------------------------------
  void AddError(std::string msg,
                std::optional<SourceLocation> loc = std::nullopt);

  // -- Constant-pool helpers ------------------------------------------
  uint32_t constant(Value v);
  uint32_t intern_name(std::string_view s);

  // -- Emit helpers ---------------------------------------------------
  template <typename T>
  inline void emit(T ins) {
    chunk_->Append(std::move(ins));
  }
  std::size_t code_size() const;

  void push_scope();
  void pop_scope();

  // -- Identifier resolution ------------------------------------------
  std::optional<std::size_t> resolve_local(const std::string& name) const;
  std::size_t add_local(const std::string& name);

  // -- Expression codegen ---------------------------------------------
  void emit_expr(std::shared_ptr<ExprAST> n);
  void emit_expr_node(const NilLiteral&);
  void emit_expr_node(const IntLiteral& n);
  void emit_expr_node(const StrLiteral& n);
  void emit_expr_node(const ListLiteral&);
  void emit_expr_node(const DictLiteral&);
  void emit_expr_node(const Identifier& n);
  void emit_expr_node(const UnaryExpr& u);
  void emit_expr_node(const BinaryExpr& b);
  void emit_expr_node(const ParenExpr& p);
  void emit_expr_node(const InvokeExpr& call);
  void emit_expr_node(const SubscriptExpr& s);
  void emit_expr_node(const MemberExpr& m);

  // -- Statement codegen ----------------------------------------------
  void emit_stmt(std::shared_ptr<AST> s);
  void emit_stmt_node(const AssignStmt& s);
  void emit_stmt_node(const AugStmt& s);
  void emit_stmt_node(const IfStmt& s);
  void emit_stmt_node(const WhileStmt& s);
  void emit_stmt_node(const ForStmt& f);
  void emit_stmt_node(const BlockStmt& s);
  void emit_stmt_node(const FuncDecl& fn);
  void emit_stmt_node(const ClassDecl& cd);
  void emit_stmt_node(const ReturnStmt& r);
  void emit_stmt_node(const std::shared_ptr<ExprAST>& s);

  void emit_function(const FuncDecl& fn);

  // -- Jump-patching --------------------------------------------------
  void patch(std::size_t site, std::size_t target);
};

}  // namespace m6
