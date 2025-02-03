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

#include "m6/evaluator.hpp"

#include "m6/symbol_table.hpp"

namespace m6 {

Evaluator::Evaluator(std::shared_ptr<SymbolTable> sym_tab)
    : sym_tab_(sym_tab) {}

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
