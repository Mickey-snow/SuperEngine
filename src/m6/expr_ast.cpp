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

#include "m6/expr_ast.hpp"

#include "m6/token.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

#include <format>
#include <unordered_map>

namespace m6 {

namespace {
struct ExpandArglistVisitor {
  ExpandArglistVisitor(std::vector<std::shared_ptr<ExprAST>>& arglist)
      : arglist_(arglist) {}
  auto operator()(auto& x) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::same_as<T, BinaryExpr>) {
      if (x.op == Op::Comma) {
        x.lhs->Apply(*this);
        x.rhs->Apply(*this);
        return;
      }
    }

    arglist_.emplace_back(std::make_shared<ExprAST>(std::move(x)));
  }
  std::vector<std::shared_ptr<ExprAST>>& arglist_;
};
}  // namespace

InvokeExpr::InvokeExpr(std::shared_ptr<ExprAST> in_fn,
                       std::shared_ptr<ExprAST> in_arg)
    : fn(in_fn) {
  if (in_arg == nullptr)
    return;

  in_arg->Apply(ExpandArglistVisitor(args));
}

// -----------------------------------------------------------------------
// AST node member functions
std::string BinaryExpr::DebugString() const {
  return lhs->DebugString() + ToString(op) + rhs->DebugString();
}

std::string AssignExpr::DebugString() const {
  return lhs->DebugString() + '=' + rhs->DebugString();
}

std::string UnaryExpr::DebugString() const {
  return ToString(op) + sub->DebugString();
}

std::string ParenExpr::DebugString() const {
  return '(' + sub->DebugString() + ')';
}

std::string InvokeExpr::DebugString() const {
  return fn->DebugString() + '(' +
         Join(", ", args | std::views::transform([](const auto& x) {
                      return x->DebugString();
                    })) +
         ')';
}

std::string SubscriptExpr::DebugString() const {
  return primary->DebugString() + '[' + index->DebugString() + ']';
}

std::string MemberExpr::DebugString() const {
  return primary->DebugString() + '.' + member->DebugString();
}

std::string IdExpr::DebugString() const {
  return tok->GetIf<tok::ID>()->id;
  ;
}

std::string const& IdExpr::GetID() const { return tok->GetIf<tok::ID>()->id; }

std::string ExprAST::DebugString() const {
  return this->Apply([](const auto& x) -> std::string {
    using T = std::decay_t<decltype(x)>;

    if constexpr (std::same_as<T, std::monostate>)
      return "<null>";
    else if constexpr (std::same_as<T, int>)
      return std::to_string(x);
    else if constexpr (std::same_as<T, std::string>)
      return x;
    else
      return x.DebugString();
  });
}

}  // namespace m6
