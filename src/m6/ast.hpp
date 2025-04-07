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

#include "m6/source_location.hpp"

#include <memory>
#include <variant>
#include <vector>

class Value;
using Value_ptr = std::shared_ptr<Value>;
enum class Op;

namespace m6 {

class AST;
class ExprAST;
struct Token;

// -----------------------------------------------------------------------
// Expression AST Nodes

// Literal
struct NilLiteral {
  SourceLocation loc;
  std::string DebugString() const;
};

struct IntLiteral {
  int value;
  SourceLocation loc;
  std::string DebugString() const;
};

struct StrLiteral {
  std::string_view value;
  SourceLocation loc;
  std::string DebugString() const;
};

// Identifier
struct Identifier {
  std::string_view value;
  SourceLocation loc;
  std::string DebugString() const;
};

// Binary operation node
struct BinaryExpr {
  Op op;
  std::shared_ptr<ExprAST> lhs;
  std::shared_ptr<ExprAST> rhs;
  SourceLocation op_loc, lhs_loc, rhs_loc;

  std::string DebugString() const;
};

// Unary operation node
struct UnaryExpr {
  Op op;
  std::shared_ptr<ExprAST> sub;
  SourceLocation op_loc, sub_loc;

  std::string DebugString() const;
};

// Parenthesized expression node
struct ParenExpr {
  std::shared_ptr<ExprAST> sub;
  SourceLocation loc;

  std::string DebugString() const;
};

// Function call node
struct InvokeExpr {
  std::shared_ptr<ExprAST> fn;
  std::vector<std::shared_ptr<ExprAST>> args;
  SourceLocation fn_loc;
  std::vector<SourceLocation> args_loc;

  std::string DebugString() const;
};

// Array subscripting node
struct SubscriptExpr {
  std::shared_ptr<ExprAST> primary;
  std::shared_ptr<ExprAST> index;

  SourceLocation primary_loc, idx_loc;

  std::string DebugString() const;
};

// Member access node
struct MemberExpr {
  std::shared_ptr<ExprAST> primary;
  std::shared_ptr<ExprAST> member;

  SourceLocation primary_loc, mem_loc;

  std::string DebugString() const;
};

// -----------------------------------------------------------------------
// ExprAST
using expr_variant_t = std::variant<IntLiteral,
                                    StrLiteral,
                                    Identifier,
                                    InvokeExpr,
                                    SubscriptExpr,
                                    MemberExpr,
                                    BinaryExpr,
                                    UnaryExpr,
                                    ParenExpr>;

class ExprAST {
 public:
  ExprAST(expr_variant_t var) : var_(std::move(var)) {}

  std::string DumpAST(std::string txt = "",
                      std::string prefix = "",
                      bool is_last = true) const;

  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) {
    return std::visit(std::forward<Visitor>(vis), var_);
  }
  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) const {
    return std::visit(std::forward<Visitor>(vis), var_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) {
    return std::visit<R>(std::forward<Visitor>(vis), var_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) const {
    return std::visit<R>(std::forward<Visitor>(vis), var_);
  }

  template <typename T>
  auto Get_if() {
    return std::get_if<T>(&var_);
  }

 private:
  expr_variant_t var_;
};

// -----------------------------------------------------------------------
// AST

// Simple Assignment '='
struct AssignStmt {
  std::shared_ptr<ExprAST> lhs;
  std::shared_ptr<ExprAST> rhs;
  SourceLocation lhs_loc, op_loc, rhs_loc;

  std::string DebugString() const;
};

// Compound Assignment
struct AugStmt {
  std::shared_ptr<ExprAST> lhs;
  Op op;
  std::shared_ptr<ExprAST> rhs;
  SourceLocation lhs_loc, op_loc, rhs_loc;

  std::string DebugString() const;
};

struct IfStmt {
  std::shared_ptr<ExprAST> cond;
  std::shared_ptr<AST> then;
  std::shared_ptr<AST> els;

  std::string DebugString() const;
};

struct WhileStmt {
  std::shared_ptr<ExprAST> cond;
  std::shared_ptr<AST> body;

  std::string DebugString() const;
};

struct ForStmt {
  std::shared_ptr<AST> init;
  std::shared_ptr<ExprAST> cond;
  std::shared_ptr<AST> inc;
  std::shared_ptr<AST> body;

  std::string DebugString() const;
};

struct BlockStmt {
  std::vector<std::shared_ptr<AST>> body;

  std::string DebugString() const;
};

using stmt_variant_t = std::variant<AssignStmt,
                                    AugStmt,
                                    IfStmt,
                                    WhileStmt,
                                    ForStmt,
                                    BlockStmt,
                                    std::shared_ptr<ExprAST>>;

class AST {
 public:
  AST(stmt_variant_t var) : var_(std::move(var)) {}

  std::string DumpAST(std::string txt = "",
                      std::string prefix = "",
                      bool is_last = true) const;

  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) {
    return std::visit(std::forward<Visitor>(vis), var_);
  }
  template <class Visitor>
  decltype(auto) Apply(Visitor&& vis) const {
    return std::visit(std::forward<Visitor>(vis), var_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) {
    return std::visit<R>(std::forward<Visitor>(vis), var_);
  }
  template <class R, class Visitor>
  R Apply(Visitor&& vis) const {
    return std::visit<R>(std::forward<Visitor>(vis), var_);
  }

  template <typename T>
  auto Get_if() {
    return std::get_if<T>(&var_);
  }

  template <typename T>
  bool HoldsAlternative() const {
    return std::holds_alternative<T>(var_);
  }

 private:
  stmt_variant_t var_;
};

}  // namespace m6
