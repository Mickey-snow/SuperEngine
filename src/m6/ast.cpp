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
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

#include <format>
#include <sstream>
#include <unordered_map>

namespace m6 {

// -----------------------------------------------------------------------
// AST node member functions

std::string IntLiteral::DebugString() const {
  return "IntLiteral " + std::to_string(GetValue());
}
int IntLiteral::GetValue() const { return tok->GetIf<tok::Int>()->value; }
std::string StrLiteral::DebugString() const {
  return "StrLiteral " + GetValue();
}
std::string const& StrLiteral::GetValue() const {
  return tok->GetIf<tok::Literal>()->str;
}
std::string BinaryExpr::DebugString() const {
  return "Binaryop " + ToString(op);
}
std::string UnaryExpr::DebugString() const { return "Unaryop " + ToString(op); }
std::string ParenExpr::DebugString() const { return "Parenthesis"; }
std::string InvokeExpr::DebugString() const { return "Invoke"; }
std::string SubscriptExpr::DebugString() const { return "Subscript"; }
std::string MemberExpr::DebugString() const { return "Member"; }
std::string Identifier::DebugString() const { return "ID " + GetID(); }
std::string const& Identifier::GetID() const {
  return tok->GetIf<tok::ID>()->id;
}

std::string AssignStmt::DebugString() const { return "Assign"; }
std::string const& AssignStmt::GetID() const {
  return lhs->Get_if<Identifier>()->GetID();
}
std::string AugStmt::DebugString() const {
  return "AugAssign " + ToString(GetOp());
}
std::string const& AugStmt::GetID() const {
  return lhs->Get_if<Identifier>()->GetID();
}
Op AugStmt::GetOp() const { return op_tok->GetIf<tok::Operator>()->op; }
std::string IfStmt::DebugString() const { return "If"; }
std::string WhileStmt::DebugString() const { return "While"; }

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
      oss << x.cond->Apply(Dumper(childPrefix, false));
      oss << x.then->DumpAST("then", childPrefix, x.els ? false : true);
      if (x.els)
        oss << x.els->DumpAST("else", childPrefix, true);
    }
    if constexpr (std::same_as<T, WhileStmt>) {
      oss << x.cond->Apply(Dumper(childPrefix, false));
      oss << x.body->DumpAST("body", childPrefix, true);
    }

    return oss.str();
  }
};

// -----------------------------------------------------------------------
// ExprAST
std::string ExprAST::DumpAST(std::string txt,
                             std::string pref,
                             bool isLast) const {
  std::ostringstream oss;

  bool empty = true;
  if (!pref.empty()) {
    oss << pref;
    oss << (isLast ? "└─" : "├─");
    empty = false;
  }
  if (!txt.empty()) {
    oss << std::move(txt);
    empty = false;
  }
  if (!empty)
    oss << '\n';

  std::string childPrefix = empty ? "" : (pref + (isLast ? "   " : "│  "));

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
          std::ostringstream oss;
          bool empty = true;
          if (!pref.empty()) {
            oss << pref;
            oss << (isLast ? "└─" : "├─");
            empty = false;
          }
          if (!txt.empty()) {
            oss << std::move(txt);
            empty = false;
          }
          if (!empty)
            oss << '\n';

          std::string childPrefix =
              empty ? "" : (pref + (isLast ? "   " : "│  "));

          oss << Dumper(childPrefix, true)(x);
          return oss.str();
        }
      },
      var_);
}

}  // namespace m6
