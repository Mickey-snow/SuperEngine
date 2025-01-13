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
#include <string>
#include <variant>
#include <vector>

class ExprAST;

// -----------------------------------------------------------------------
// Expression operator
enum Op : int {
  Unknown = -1,

  Comma,  // ","

  // Arithmetic Operators
  Add,  // "+"
  Sub,  // "-"
  Mul,  // "*"
  Div,  // "/"
  Mod,  // "%"

  // Bitwise Operators
  BitAnd,      // "&"
  BitOr,       // "|"
  BitXor,      // "^"
  ShiftLeft,   // "<<"
  ShiftRight,  // ">>"
  Tilde,       // "~"

  // Compound Assignment Operators
  AddAssign,         // "+="
  SubAssign,         // "-="
  MulAssign,         // "*="
  DivAssign,         // "/="
  ModAssign,         // "%="
  BitAndAssign,      // "&="
  BitOrAssign,       // "|="
  BitXorAssign,      // "^="
  ShiftLeftAssign,   // "<<="
  ShiftRightAssign,  // ">>="

  // Assignment Operator
  Assign,  // "="

  // Comparison Operators
  Equal,         // "=="
  NotEqual,      // "!="
  LessEqual,     // "<="
  Less,          // "<"
  GreaterEqual,  // ">="
  Greater,       // ">"

  // Logical Operators
  LogicalAnd,  // "&&"
  LogicalOr    // "||"
};
std::string ToString(Op op);

// -----------------------------------------------------------------------
// AST Nodes

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
  std::string id;
  std::shared_ptr<ExprAST> idx;

  std::string DebugString() const;
};

// -----------------------------------------------------------------------
// AST
using expr_variant_t = std::variant<std::monostate,  // null
                                    int,             // integer literal
                                    std::string,     // identifier
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
  std::string operator()(const BinaryExpr& x) const {
    return ToString(x.op) + ' ' + x.lhs->Apply(*this) + ' ' +
           x.rhs->Apply(*this);
  }
  std::string operator()(const UnaryExpr& x) const {
    return ToString(x.op) + ' ' + x.sub->Apply(*this);
  }
  std::string operator()(const ParenExpr& x) const {
    return x.sub->Apply(*this);
  }
  std::string operator()(const ReferenceExpr& x) const {
    return x.id + '[' + x.idx->Apply(*this) + ']';
  }
  std::string operator()(std::monostate) const { return "<null>"; }
  std::string operator()(int x) const { return std::to_string(x); }
  std::string operator()(const std::string& str) const { return str; }
};
