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
#include "m6/token.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"

namespace m6 {

void Compiler::AddNative(Value fn) {
  auto ptr = fn.Get_if<NativeFunction>();
  if (!ptr)
    throw std::runtime_error("Compiler: " + fn.Desc() +
                             " is not a native function.");
  native_fn_[ptr->Name()] = std::move(fn);
}

std::vector<Instruction> Compiler::Compile(std::shared_ptr<ExprAST> expr) {
  struct Visitor {
    void operator()(std::monostate) { bk = Push(Value(std::monostate())); }
    void operator()(const IdExpr& idexpr) {
      std::string const& id = idexpr.GetID();
      auto it = compiler.local_variable_.find(id);
      if (it == compiler.local_variable_.cend()) {
        throw NameError("Name '" + id + "' not defined.", idexpr.tok);
      }

      bk = Load(it->second);
    }
    void operator()(int x) { bk = Push(Value(x)); }
    void operator()(const std::string& x) { bk = Push(Value(x)); }
    void operator()(const InvokeExpr& x) {
      for (auto arg : x.args)
        arg->Apply(*this);

      if (auto idexpr = x.fn->Get_if<IdExpr>()) {
        std::string const& id = idexpr->GetID();
        auto it = compiler.native_fn_.find(id);
        if (it == compiler.native_fn_.cend())
          throw NameError("Name '" + id + "' is not defined.", idexpr->tok);

        bk = Push(it->second);
        bk = Invoke(x.args.size());
      } else
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
    Compiler& compiler;
  };

  std::vector<Instruction> result;

  // special case: variable declaration, this should be a separate statement
  // type.
  // <id> = <expr>
  if (auto assign = expr->Get_if<AssignExpr>()) {
    auto idtok = assign->lhs->Get_if<IdExpr>();
    if (!idtok)
      throw SyntaxError("Cannot assign to expression here.");
    assign->rhs->Apply(Visitor(std::back_inserter(result), *this));

    std::string const& id = idtok->GetID();
    if (local_variable_.contains(id)) {
      result.push_back(Store(local_variable_[id]));
      result.push_back(Pop());
    } else {
      local_variable_[id] = local_variable_.size();
    }

  } else {
    // regular expression
    expr->Apply(Visitor(std::back_inserter(result), *this));
  }

  return result;
}

}  // namespace m6
