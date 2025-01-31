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

namespace m6 {

class ExprAST;
class Value;
enum class Op;

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

// Memory reference expression node
struct ReferenceExpr {
  IdExpr id;
  std::shared_ptr<ExprAST> idx;

  std::string DebugString() const;
};

// -----------------------------------------------------------------------
// AST
using expr_variant_t = std::variant<std::monostate,  // null
                                    int,             // integer literal
                                    std::string,     // string literal
                                    IdExpr,          // identifier
                                    ReferenceExpr,
                                    BinaryExpr,
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

// -----------------------------------------------------------------------
// AST visitors

struct GetPrefix {
  std::string operator()(const BinaryExpr& x) const;
  std::string operator()(const UnaryExpr& x) const;
  std::string operator()(const ParenExpr& x) const;
  std::string operator()(const ReferenceExpr& x) const;
  std::string operator()(std::monostate) const;
  std::string operator()(int x) const;
  std::string operator()(const std::string& str) const;
  std::string operator()(const IdExpr& str) const;
};

struct Evaluator {
  Value operator()(std::monostate) const;
  Value operator()(const IdExpr& str) const;
  Value operator()(int x) const;
  Value operator()(const std::string& x) const;
  Value operator()(const ReferenceExpr& x) const;
  Value operator()(const ParenExpr& x) const;
  Value operator()(const UnaryExpr& x) const;
  Value operator()(const BinaryExpr& x) const;
};

}  // namespace m6
