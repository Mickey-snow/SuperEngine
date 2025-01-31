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
#include "m6/value.hpp"

#include <format>
#include <unordered_map>

namespace m6 {

// -----------------------------------------------------------------------
// enum Op helper methods
std::string ToString(Op op) {
  switch (op) {
    case Op::Comma:
      return ",";
    case Op::Add:
      return "+";
    case Op::Sub:
      return "-";
    case Op::Mul:
      return "*";
    case Op::Div:
      return "/";
    case Op::Mod:
      return "%";
    case Op::BitAnd:
      return "&";
    case Op::BitOr:
      return "|";
    case Op::BitXor:
      return "^";
    case Op::ShiftLeft:
      return "<<";
    case Op::ShiftRight:
      return ">>";
    case Op::Tilde:
      return "~";
    case Op::AddAssign:
      return "+=";
    case Op::SubAssign:
      return "-=";
    case Op::MulAssign:
      return "*=";
    case Op::DivAssign:
      return "/=";
    case Op::ModAssign:
      return "%=";
    case Op::BitAndAssign:
      return "&=";
    case Op::BitOrAssign:
      return "|=";
    case Op::BitXorAssign:
      return "^=";
    case Op::ShiftLeftAssign:
      return "<<=";
    case Op::ShiftRightAssign:
      return ">>=";
    case Op::Assign:
      return "=";
    case Op::Equal:
      return "==";
    case Op::NotEqual:
      return "!=";
    case Op::LessEqual:
      return "<=";
    case Op::Less:
      return "<";
    case Op::GreaterEqual:
      return ">=";
    case Op::Greater:
      return ">";
    case Op::LogicalAnd:
      return "&&";
    case Op::LogicalOr:
      return "||";
    default:
      return std::format("<op:{}>", static_cast<int>(op));
  }
}
Op CreateOp(std::string_view str) {
  static const std::unordered_map<std::string_view, Op> strop_table{
      {",", Op::Comma},
      {"+", Op::Add},
      {"-", Op::Sub},
      {"*", Op::Mul},
      {"/", Op::Div},
      {"%", Op::Mod},
      {"&", Op::BitAnd},
      {"|", Op::BitOr},
      {"^", Op::BitXor},
      {"<<", Op::ShiftLeft},
      {">>", Op::ShiftRight},
      {"~", Op::Tilde},
      {"+=", Op::AddAssign},
      {"-=", Op::SubAssign},
      {"*=", Op::MulAssign},
      {"/=", Op::DivAssign},
      {"%=", Op::ModAssign},
      {"&=", Op::BitAndAssign},
      {"|=", Op::BitOrAssign},
      {"^=", Op::BitXorAssign},
      {"<<=", Op::ShiftLeftAssign},
      {">>=", Op::ShiftRightAssign},
      {"=", Op::Assign},
      {"==", Op::Equal},
      {"!=", Op::NotEqual},
      {"<=", Op::LessEqual},
      {"<", Op::Less},
      {">=", Op::GreaterEqual},
      {">", Op::Greater},
      {"&&", Op::LogicalAnd},
      {"||", Op::LogicalOr}};

  if (strop_table.contains(str))
    return strop_table.at(str);
  else
    return Op::Unknown;
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

std::string ReferenceExpr::DebugString() const {
  return id.id + idx->DebugString();
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
std::string GetPrefix::operator()(const ReferenceExpr& x) const {
  return x.id.id + '[' + x.idx->Apply(*this) + ']';
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
Value Evaluator::operator()(const ReferenceExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value Evaluator::operator()(const ParenExpr& x) const {
  return x.sub->Apply(*this);
}
Value Evaluator::operator()(const UnaryExpr& x) const {
  return Value::Calculate(x.op, x.sub->Apply(*this));
}
Value Evaluator::operator()(const BinaryExpr& x) const {
  return Value::Calculate(x.lhs->Apply(*this), x.op, x.rhs->Apply(*this));
}

}  // namespace m6
