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

#include "m6/op.hpp"
#include "m6/value.hpp"
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
// method DebugString
std::string BinaryExpr::DebugString() const {
  return lhs->DebugString() + ToString(op) + rhs->DebugString();
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

std::string IdExpr::DebugString() const { return id; }

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

// -----------------------------------------------------------------------
// struct GetPrefix
std::string GetPrefix::operator()(const BinaryExpr& x) const {
  return ToString(x.op) + ' ' + x.lhs->Apply(*this) + ' ' + x.rhs->Apply(*this);
}
std::string GetPrefix::operator()(const UnaryExpr& x) const {
  return ToString(x.op) + ' ' + x.sub->Apply(*this);
}
std::string GetPrefix::operator()(const ParenExpr& x) const {
  return x.sub->Apply(*this);
}
std::string GetPrefix::operator()(const InvokeExpr& x) const {
  return x.fn->Apply(*this) + '(' +
         Join(", ", x.args | std::views::transform([&](const auto& arg) {
                      return arg->Apply(*this);
                    })) +
         ')';
}
std::string GetPrefix::operator()(const SubscriptExpr& x) const {
  return x.primary->Apply(*this) + '[' + x.index->Apply(*this) + ']';
}
std::string GetPrefix::operator()(const MemberExpr& x) const {
  return x.primary->Apply(*this) + '.' + x.member->Apply(*this);
}
std::string GetPrefix::operator()(std::monostate) const { return "<null>"; }
std::string GetPrefix::operator()(int x) const { return std::to_string(x); }
std::string GetPrefix::operator()(const std::string& str) const {
  return '"' + str + '"';
}
std::string GetPrefix::operator()(const IdExpr& id) const { return id.id; }

// -----------------------------------------------------------------------
// struct Evaluator
Value Evaluator::operator()(std::monostate) const {
  return make_value(nullptr);
}
Value Evaluator::operator()(const IdExpr& str) const {
  throw std::runtime_error("not supported yet.");
}
Value Evaluator::operator()(int x) const { return make_value(x); }
Value Evaluator::operator()(const std::string& x) const {
  return make_value(x);
}
Value Evaluator::operator()(const InvokeExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value Evaluator::operator()(const SubscriptExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value Evaluator::operator()(const MemberExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value Evaluator::operator()(const ParenExpr& x) const {
  return x.sub->Apply(*this);
}
Value Evaluator::operator()(const UnaryExpr& x) const {
  Value rhs = x.sub->Apply(*this);
  return rhs->Operator(x.op);
}
Value Evaluator::operator()(const BinaryExpr& x) const {
  Value rhs = x.rhs->Apply(*this);
  Value lhs = x.lhs->Apply(*this);
  return lhs->Operator(x.op, rhs);
}

}  // namespace m6
