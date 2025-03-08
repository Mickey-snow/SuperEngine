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

#include "m6/compiler.hpp"

#include "m6/expr_ast.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"

namespace m6 {

std::vector<Instruction> Compile(std::shared_ptr<ExprAST> expr) {
  struct Visitor {
    void operator()(std::monostate) { bk = Push(Value(std::monostate())); }
    void operator()(const IdExpr& idexpr) {
      throw std::runtime_error("not supported yet.");
    }
    void operator()(int x) { bk = Push(Value(x)); }
    void operator()(const std::string& x) { bk = Push(Value(x)); }
    void operator()(const InvokeExpr& x) {
      throw std::runtime_error("not supported yet.");
    }
    void operator()(const SubscriptExpr& x) {
      throw std::runtime_error("not supported yet.");
    }
    void operator()(const MemberExpr& x) {
      throw std::runtime_error("not supported yet.");
    }
    void operator()(const ParenExpr& x) { return x.sub->Apply(*this); }
    void operator()(const UnaryExpr& x) {
      x.sub->Apply(*this);
      bk = UnaryOp(x.op);
    }
    void operator()(const BinaryExpr& x) {
      x.rhs->Apply(*this);
      x.lhs->Apply(*this);
      bk = BinaryOp(x.op);
    }
    void operator()(const AssignExpr& x) {
      throw std::runtime_error("not supported yet.");
    }

    std::back_insert_iterator<std::vector<Instruction>> bk;
  };

  std::vector<Instruction> result;
  expr->Apply(Visitor(std::back_inserter(result)));
  return result;
}

}  // namespace m6
