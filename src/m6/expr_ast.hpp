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

// -----------------------------------------------------------------------
// AST Nodes

// Identifier
struct IdExpr {
  std::string id;

  std::string DebugString() const;
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
  InvokeExpr(std::shared_ptr<ExprAST> in_fn, std::shared_ptr<ExprAST> in_arg);

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
using expr_variant_t = std::variant<std::monostate,  // null
                                    int,             // integer literal
                                    std::string,     // string literal
                                    IdExpr,          // identifier
                                    InvokeExpr,
                                    SubscriptExpr,
                                    MemberExpr,
                                    BinaryExpr,
                                    AssignExpr,
                                    UnaryExpr,
                                    ParenExpr>;

class ExprAST {
 public:
  ExprAST() : var_(std::monostate()) {}
  ExprAST(expr_variant_t var) : var_(std::move(var)) {}

  std::string DebugString() const;

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

 private:
  expr_variant_t var_;
};

}  // namespace m6
