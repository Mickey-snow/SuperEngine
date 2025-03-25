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

#include "m6/ast.hpp"
#include "m6/exception.hpp"
#include "m6/token.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"

#include <optional>

namespace m6 {

void Compiler::AddNative(Value fn) {
  auto ptr = fn.Get_if<NativeFunction>();
  if (!ptr)
    throw std::runtime_error("Compiler: " + fn.Desc() +
                             " is not a native function.");
  native_fn_[ptr->Name()] = std::move(fn);
}

std::vector<Instruction> Compiler::Compile(std::shared_ptr<ExprAST> expr) {
  std::vector<Instruction> result;
  Compile(expr, result);
  return result;
}
void Compiler::Compile(std::shared_ptr<ExprAST> expr,
                       std::vector<Instruction>& result) {
  struct Visitor {
    Compiler& compiler;
    std::vector<Instruction>& result;

    inline void Emit(Instruction i) { result.emplace_back(std::move(i)); }

    void operator()(const Identifier& idexpr) {
      std::string const& id = idexpr.GetID();
      auto it = compiler.local_variable_.find(id);
      if (it == compiler.local_variable_.cend()) {
        throw NameError("Name '" + id + "' not defined.", *idexpr.tok);
      }

      Emit(Load(it->second));
    }
    void operator()(const IntLiteral& x) { Emit(Push(Value(x.GetValue()))); }
    void operator()(const StrLiteral& x) { Emit(Push(Value(x.GetValue()))); }
    void operator()(const InvokeExpr& x) {
      for (auto arg : x.args)
        arg->Apply(*this);

      if (auto idexpr = x.fn->Get_if<Identifier>()) {
        std::string const& id = idexpr->GetID();
        auto it = compiler.native_fn_.find(id);
        if (it == compiler.native_fn_.cend())
          throw NameError("Name '" + id + "' is not defined.", *idexpr->tok);

        Emit(Push(it->second));
        Emit(Invoke(x.args.size()));
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
      Emit(UnaryOp(x.op));
    }
    void operator()(const BinaryExpr& x) {
      x.rhs->Apply(*this);
      x.lhs->Apply(*this);
      Emit(BinaryOp(x.op));
    }
  };

  expr->Apply(Visitor(*this, result));
}

std::vector<Instruction> Compiler::Compile(std::shared_ptr<AST> stmt) {
  std::vector<Instruction> result;
  Compile(stmt, result);
  return result;
}
void Compiler::Compile(std::shared_ptr<AST> stmt,
                       std::vector<Instruction>& result) {
  struct Visitor {
    Compiler& compiler;
    std::vector<Instruction>& result;

    inline void Emit(Instruction instruction) {
      result.emplace_back(std::move(instruction));
    }

    void operator()(const AssignExpr& x) {
      std::string const& id = x.GetID();

      compiler.Compile(x.rhs, result);
      if (compiler.local_variable_.contains(id)) {
        Emit(Store(compiler.local_variable_.at(id)));
        Emit(Pop());
      } else {
        compiler.local_variable_[id] = compiler.local_variable_.size();
      }
    }
    void operator()(const AugExpr& x) {
      auto it = compiler.local_variable_.find(x.GetID());
      if (it == compiler.local_variable_.cend())
        throw NameError("Name '" + x.GetID() + "' not defined.",
                        std::make_optional<SourceLocation>(*(x.lhs->tok)));

      Emit(Load(it->second));
      compiler.Compile(x.rhs, result);
      switch (x.GetOp()) {
        case Op::AddAssign:
          Emit(BinaryOp(Op::Add));
          break;
        case Op::SubAssign:
          Emit(BinaryOp(Op::Sub));
          break;
        case Op::MulAssign:
          Emit(BinaryOp(Op::Mul));
          break;
        case Op::DivAssign:
          Emit(BinaryOp(Op::Div));
          break;
        case Op::Mod:
          Emit(BinaryOp(Op::Mod));
          break;
        case Op::BitAnd:
          Emit(BinaryOp(Op::BitAnd));
          break;
        case Op::BitOr:
          Emit(BinaryOp(Op::BitOr));
          break;
        case Op::BitXor:
          Emit(BinaryOp(Op::BitXor));
          break;
        case Op::ShiftLeft:
          Emit(BinaryOp(Op::ShiftLeft));
          break;
        case Op::ShiftRight:
          Emit(BinaryOp(Op::ShiftRight));
          break;
        case Op::ShiftUnsignedRight:
          Emit(BinaryOp(Op::ShiftUnsignedRight));
          break;
        default:
          throw std::runtime_error("Compiler: Unknown operator '" +
                                   ToString(x.GetOp()) + "' in AugExpr.");
      }
      Emit(Store(it->second));
      Emit(Pop());
    }
    void operator()(const std::shared_ptr<ExprAST>& x) {
      compiler.Compile(x, result);
    }
  };

  stmt->Apply(Visitor(*this, result));
}

}  // namespace m6
