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

#include "log/domain_logger.hpp"
#include "m6/exception.hpp"
#include "m6/symbol_table.hpp"

#include <algorithm>

namespace m6 {

static DomainLogger logger("Evaluator");

Evaluator::Evaluator(std::shared_ptr<SymbolTable> sym_tab)
    : sym_tab_(sym_tab) {}

Value_ptr Evaluator::operator()(std::monostate) const {
  logger(Severity::Warn) << "Evaluating nil";
  return make_value(nullptr);
}
Value_ptr Evaluator::operator()(const IdExpr& idexpr) const {
  if (sym_tab_ == nullptr)
    throw std::runtime_error("Evaluator: no symbol table avaliable.");
  return sym_tab_->Get(idexpr.id);
}
Value_ptr Evaluator::operator()(int x) const { return make_value(x); }
Value_ptr Evaluator::operator()(const std::string& x) const {
  return make_value(x);
}
Value_ptr Evaluator::operator()(const InvokeExpr& x) const {
  Value_ptr fn = x.fn->Apply(*this);
  std::vector<Value_ptr> args;
  args.reserve(x.args.size());
  std::transform(x.args.cbegin(), x.args.cend(), std::back_inserter(args),
                 [&](const auto arg_ast) { return arg_ast->Apply(*this); });
  std::map<std::string, Value_ptr> kwargs;
  return fn->Invoke(args);
}
Value_ptr Evaluator::operator()(const SubscriptExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value_ptr Evaluator::operator()(const MemberExpr& x) const {
  throw std::runtime_error("not supported yet.");
}
Value_ptr Evaluator::operator()(const ParenExpr& x) const {
  return x.sub->Apply(*this);
}
Value_ptr Evaluator::operator()(const UnaryExpr& x) const {
  Value_ptr rhs = x.sub->Apply(*this);
  return rhs->__Operator(x.op);
}
Value_ptr Evaluator::operator()(const BinaryExpr& x) const {
  Value_ptr rhs = x.rhs->Apply(*this);
  Value_ptr lhs = x.lhs->Apply(*this);
  return lhs->__Operator(x.op, rhs);
}
Value_ptr Evaluator::operator()(const AssignExpr& x) const {
  // resolve address
  if (sym_tab_ == nullptr)
    throw std::runtime_error("Evaluator: no symbol table avaliable.");
  const std::string varname = x.lhs->Apply([](const auto& x) -> std::string {
    using T = std::decay_t<decltype(x)>;

    if constexpr (std::same_as<T, IdExpr>)
      return x.id;

    throw m6::SyntaxError("Cannot assign.");
  });

  Value_ptr value = x.rhs->Apply(*this);
  sym_tab_->Set(varname, value.unique() ? value : value->Duplicate());
  return value;
}

}  // namespace m6
