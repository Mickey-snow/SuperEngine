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

#include "m6/ast.hpp"

#include "m6/token.hpp"
#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"

#include <format>
#include <sstream>
#include <unordered_map>

namespace m6 {

// -----------------------------------------------------------------------
// AST node member functions

std::string IntLiteral::DebugString() const {
  return "IntLiteral " + std::to_string(value);
}
std::string StrLiteral::DebugString() const {
  return "StrLiteral " + std::string(value);
}
std::string BinaryExpr::DebugString() const {
  return "Binaryop " + ToString(op);
}
std::string UnaryExpr::DebugString() const { return "Unaryop " + ToString(op); }
std::string ParenExpr::DebugString() const { return "Parenthesis"; }
std::string InvokeExpr::DebugString() const { return "Invoke"; }
std::string SubscriptExpr::DebugString() const { return "Subscript"; }
std::string MemberExpr::DebugString() const { return "Member"; }
std::string Identifier::DebugString() const {
  return "ID " + std::string(value);
}

std::string AssignStmt::DebugString() const { return "Assign"; }
std::string AugStmt::DebugString() const { return "AugAssign " + ToString(op); }
Op AugStmt::GetRmAssignmentOp() const {
  switch (op) {
    case Op::AddAssign:
      return Op::Add;
    case Op::SubAssign:
      return Op::Sub;
    case Op::MulAssign:
      return Op::Mul;
    case Op::DivAssign:
      return Op::Div;
    case Op::Mod:
      return Op::Mod;
    case Op::BitAnd:
      return Op::BitAnd;
    case Op::BitOr:
      return Op::BitOr;
    case Op::BitXor:
      return Op::BitXor;
    case Op::ShiftLeft:
      return Op::ShiftLeft;
    case Op::ShiftRight:
      return Op::ShiftRight;
    case Op::ShiftUnsignedRight:
      return Op::ShiftUnsignedRight;
    default:
      return op;
  }
}
std::string IfStmt::DebugString() const { return "If"; }
std::string WhileStmt::DebugString() const { return "While"; }
std::string ForStmt::DebugString() const { return "For"; }
std::string BlockStmt::DebugString() const { return "Compound"; }
std::string FuncDecl::DebugString() const {
  return std::format("fn {}({})", name, Join(",", params));
}
std::string ClassDecl::DebugString() const {
  return "class " + std::string(name);
}
std::string ReturnStmt::DebugString() const { return "return"; }

// -----------------------------------------------------------------------
// Visitor to print debug string for an AST
struct Dumper {
  std::string pref;
  bool isLast;

  std::string operator()(const auto& x) const {
    using T = std::decay_t<decltype(x)>;

    std::ostringstream oss;
    if (!pref.empty()) {
      oss << pref;
      oss << (isLast ? "└─" : "├─");
    }
    oss << x.DebugString() << '\n';

    std::string childPrefix = pref + (isLast ? "   " : "│  ");

    if constexpr (std::same_as<T, BinaryExpr>) {
      oss << x.lhs->Apply(Dumper(childPrefix, false));
      oss << x.rhs->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, AugStmt> || std::same_as<T, AssignStmt>) {
      oss << x.lhs->Apply(Dumper(childPrefix, false));
      oss << x.rhs->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, UnaryExpr> || std::same_as<T, ParenExpr>) {
      oss << x.sub->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, InvokeExpr>) {
      oss << x.fn->Apply(Dumper(childPrefix, false));
      for (size_t i = 0; i < x.args.size(); ++i)
        oss << x.args[i]->Apply(
            Dumper(childPrefix, (i == x.args.size() - 1 ? true : false)));
    }
    if constexpr (std::same_as<T, SubscriptExpr>) {
      oss << x.primary->Apply(Dumper(childPrefix, false));
      oss << x.index->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, MemberExpr>) {
      oss << x.primary->Apply(Dumper(childPrefix, false));
      oss << x.member->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, IfStmt>) {
      oss << x.cond->DumpAST("cond", childPrefix, false);
      oss << x.then->DumpAST("then", childPrefix, x.els ? false : true);
      if (x.els)
        oss << x.els->DumpAST("else", childPrefix, true);
    }
    if constexpr (std::same_as<T, WhileStmt>) {
      oss << x.cond->DumpAST("cond", childPrefix, false);
      oss << x.body->DumpAST("body", childPrefix, true);
    }
    if constexpr (std::same_as<T, ForStmt>) {
      oss << x.init->DumpAST("init", childPrefix, false);
      oss << x.cond->DumpAST("cond", childPrefix, false);
      oss << x.inc->DumpAST("inc", childPrefix, false);
      oss << x.body->DumpAST("body", childPrefix, true);
    }
    if constexpr (std::same_as<T, BlockStmt>) {
      for (size_t i = 0; i < x.body.size(); ++i)
        oss << x.body[i]->DumpAST("", childPrefix, i + 1 >= x.body.size());
    }
    if constexpr (std::same_as<T, FuncDecl>) {
      oss << x.body->DumpAST("body", childPrefix, true);
    }
    if constexpr (std::same_as<T, ClassDecl>) {
      for (size_t i = 0; i < x.members.size(); ++i) {
        Dumper dumper(childPrefix, i + 1 >= x.members.size());
        oss << dumper(x.members[i]);
      }
    }
    if constexpr (std::same_as<T, ReturnStmt>) {
      if (x.value)
        oss << x.value->Apply(Dumper(childPrefix, true));
    }

    return oss.str();
  }
};

// -----------------------------------------------------------------------
// ExprAST
std::string ExprAST::DumpAST(std::string txt,
                             std::string pref,
                             bool isLast) const {
  if (txt.empty())
    return std::visit(Dumper(pref, isLast), var_);

  std::ostringstream oss;

  if (!pref.empty()) {
    oss << pref;
    oss << (isLast ? "└─" : "├─");
  }
  if (!txt.empty())
    oss << txt << '\n';

  std::string childPrefix = pref + (isLast ? "   " : "│  ");
  oss << std::visit(Dumper(childPrefix, true), var_);
  return oss.str();
}

// -----------------------------------------------------------------------
// AST
std::string AST::DumpAST(std::string txt, std::string pref, bool isLast) const {
  return std::visit(
      [&](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::same_as<T, std::shared_ptr<ExprAST>>)
          return x->DumpAST(std::move(txt), std::move(pref), isLast);
        else {
          if (txt.empty())
            return Dumper(pref, isLast)(x);

          std::ostringstream oss;
          if (!pref.empty()) {
            oss << pref;
            oss << (isLast ? "└─" : "├─");
          }
          oss << txt << '\n';

          std::string childPrefix = pref + (isLast ? "   " : "│  ");

          oss << Dumper(childPrefix, true)(x);
          return oss.str();
        }
      },
      var_);
}

}  // namespace m6
