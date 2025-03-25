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

std::string AssignExpr::DebugString() const { return "Assign"; }
std::string const& AssignExpr::GetID() const { return lhs->GetID(); }

std::string AugExpr::DebugString() const {
  return "AugAssign " + ToString(GetOp());
}
std::string const& AugExpr::GetID() const { return lhs->GetID(); }
Op AugExpr::GetOp() const { return op_tok->GetIf<tok::Operator>()->op; }

std::string UnaryExpr::DebugString() const { return "Unaryop " + ToString(op); }

std::string ParenExpr::DebugString() const { return "Parenthesis"; }

std::string InvokeExpr::DebugString() const { return "Invoke"; }

std::string SubscriptExpr::DebugString() const { return "Subscript"; }

std::string MemberExpr::DebugString() const { return "Member"; }

std::string Identifier::DebugString() const { return "ID " + GetID(); }
std::string const& Identifier::GetID() const {
  return tok->GetIf<tok::ID>()->id;
}

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
    if constexpr (std::same_as<T, std::shared_ptr<ExprAST>>) {
      oss << x->Apply(*this) << '\n';
      return oss.str();
    } else {
      oss << x.DebugString() << '\n';
    }

    std::string childPrefix = pref + (isLast ? "   " : "│  ");

    if constexpr (std::same_as<T, BinaryExpr>) {
      oss << x.lhs->Apply(Dumper(childPrefix, false));
      oss << x.rhs->Apply(Dumper(childPrefix, true));
    }
    if constexpr (std::same_as<T, AugExpr> || std::same_as<T, AssignExpr>) {
      oss << Dumper(childPrefix, false)(*x.lhs);
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

    return oss.str();
  }
};

// -----------------------------------------------------------------------
// ExprAST
std::string ExprAST::DumpAST() const {
  return std::visit(Dumper("", true), var_);
}

// -----------------------------------------------------------------------
// AST
std::string AST::DumpAST() const { return std::visit(Dumper("", true), var_); }

}  // namespace m6
