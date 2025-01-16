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

#include "base/expr_ast.hpp"

#include <format>
#include <unordered_map>

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
  return id + idx->DebugString();
}

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
  return x.id + '[' + x.idx->Apply(*this) + ']';
}
std::string GetPrefix::operator()(std::monostate) const { return "<null>"; }
std::string GetPrefix::operator()(int x) const { return std::to_string(x); }
std::string GetPrefix::operator()(const std::string& str) const { return str; }

// -----------------------------------------------------------------------
// struct Evaluator
int Evaluator::operator()(std::monostate) const {
  throw std::runtime_error("Evaluator: <null> found in ast.");
}
int Evaluator::operator()(const std::string& str) const {
  throw std::runtime_error("not supported yet.");
}
int Evaluator::operator()(int x) const { return x; }
int Evaluator::operator()(const ReferenceExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
int Evaluator::operator()(const ParenExpr& x) const {
  return x.sub->Apply(*this);
}
int Evaluator::operator()(const UnaryExpr& x) const {
  auto rhs = x.sub->Apply(*this);
  switch (x.op) {
    case Op::Add:
      return rhs;
    case Op::Sub:
      return -rhs;
    case Op::Tilde:
      return ~rhs;
    default:
      throw std::runtime_error("Evaluator: unsupported operator '" +
                               ToString(x.op) + "' found in unary expression.");
  }
}
int Evaluator::operator()(const BinaryExpr& x) const {
  auto lhs = x.lhs->Apply(*this);
  auto rhs = x.rhs->Apply(*this);

  switch (x.op) {
    case Comma:
      return rhs;

    case Add:
      return lhs + rhs;
    case Sub:
      return lhs - rhs;
    case Mul:
      return lhs * rhs;
    case Div: {
      if (rhs == 0)
        return 0;
      return lhs / rhs;
    }
    case Mod:
      return lhs % rhs;
    case BitAnd:
      return lhs & rhs;
    case BitOr:
      return lhs | rhs;
    case BitXor:
      return lhs ^ rhs;
    case ShiftLeft:
      return lhs << rhs;
    case ShiftRight:
      return lhs >> rhs;

    case Equal:
      return lhs == rhs ? 1 : 0;
    case NotEqual:
      return lhs != rhs ? 1 : 0;
    case LessEqual:
      return lhs <= rhs ? 1 : 0;
    case Less:
      return lhs < rhs ? 1 : 0;
    case GreaterEqual:
      return lhs >= rhs ? 1 : 0;
    case Greater:
      return lhs > rhs ? 1 : 0;

    case LogicalAnd:
      return lhs && rhs ? 1 : 0;
    case LogicalOr:
      return lhs || rhs ? 1 : 0;

    default:
      throw std::runtime_error("Evaluator: unsupported operator '" +
                               ToString(x.op) +
                               "' found in binary expression.");
  }
}
