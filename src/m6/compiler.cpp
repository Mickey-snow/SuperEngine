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

#include "m6/exception.hpp"
#include "m6/expr_ast.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"

namespace m6 {

std::vector<Instruction> Compiler::Compile(std::shared_ptr<ExprAST> expr) {
  struct Visitor {
    void operator()(std::monostate) { bk = Push(Value(std::monostate())); }
    void operator()(const IdExpr& idexpr) {
      auto it = symbol_table.find(idexpr.id);
      if (it == symbol_table.cend()) {
        throw NameError("Name '" + idexpr.id + "' not defined.");
      }

      bk = Load(it->second);
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
      // this is inside an expression
      throw SyntaxError("Invalid syntax.");
    }

    std::back_insert_iterator<std::vector<Instruction>> bk;
    std::map<std::string, size_t>& symbol_table;
  };

  std::vector<Instruction> result;

  // special case: variable declaration, this should be a separate statement
  // type.
  // <id> = <expr>
  if (auto assign = expr->Get_if<AssignExpr>()) {
    auto id = assign->lhs->Get_if<IdExpr>();
    if (!id)
      throw SyntaxError("Cannot assign to expression here.");
    assign->rhs->Apply(Visitor(std::back_inserter(result), local_variable_));

    if (local_variable_.contains(id->id)) {
      result.push_back(Store(local_variable_[id->id]));
      result.push_back(Pop());
    } else {
      local_variable_[id->id] = local_variable_.size();
    }

  } else {
    // regular expression
    expr->Apply(Visitor(std::back_inserter(result), local_variable_));
  }

  return result;
}

}  // namespace m6
