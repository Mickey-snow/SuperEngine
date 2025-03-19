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

#include <memory>
#include <variant>
#include <vector>

class Value;
using Value_ptr = std::shared_ptr<Value>;
enum class Op;

namespace m6 {

class ExprAST;
class Token;

// -----------------------------------------------------------------------
// AST Nodes

// Literal
struct NilLiteral {
  Token* tok;
  std::string DebugString() const;
};

struct IntLiteral {
  Token* tok;
  std::string DebugString() const;
  int GetValue() const;
};

struct StrLiteral {
  Token* tok;
  std::string DebugString() const;
  std::string const& GetValue() const;
};

// Identifier
struct Identifier {
  Token* tok;
  std::string DebugString() const;
  std::string const& GetID() const;
};

// Binary operation node
struct BinaryExpr {
  Op op;
  std::shared_ptr<ExprAST> lhs;
  std::shared_ptr<ExprAST> rhs;

  std::string DebugString() const;
};

// Simple Assignment '='
struct AssignExpr {
  std::shared_ptr<ExprAST> lhs;
  std::shared_ptr<ExprAST> rhs;

  std::string DebugString() const;
};

// Compound Assignment
struct AugExpr {
  Identifier* lhs;
  Token* op_tok;
  std::shared_ptr<ExprAST> rhs;

  std::string DebugString() const;
  std::string const& GetID() const;
  Op GetOp() const;
};

// Unary operation node
struct UnaryExpr {
  Op op;
  std::shared_ptr<ExprAST> sub;

  std::string DebugString() const;
};

// Parenthesized expression node
struct ParenExpr {
  std::shared_ptr<ExprAST> sub;

  std::string DebugString() const;
};

// Function call node
struct InvokeExpr {
  std::shared_ptr<ExprAST> fn;
  std::vector<std::shared_ptr<ExprAST>> args;

  std::string DebugString() const;
};

// Array subscripting node
struct SubscriptExpr {
  std::shared_ptr<ExprAST> primary;
  std::shared_ptr<ExprAST> index;

  std::string DebugString() const;
};

// Member access node
struct MemberExpr {
  std::shared_ptr<ExprAST> primary;
  std::shared_ptr<ExprAST> member;

  std::string DebugString() const;
};

// -----------------------------------------------------------------------
// AST
using expr_variant_t = std::variant<IntLiteral,
                                    StrLiteral,
                                    Identifier,
                                    InvokeExpr,
                                    SubscriptExpr,
                                    MemberExpr,
                                    BinaryExpr,
                                    AssignExpr,
                                    AugExpr,
                                    UnaryExpr,
                                    ParenExpr>;

class ExprAST {
 public:
  ExprAST(expr_variant_t var) : var_(std::move(var)) {}

  std::string DumpAST() const;

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

}  // namespace m6
