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

#pragma once

#include "m6/expr_ast.hpp"
#include "m6/value.hpp"

namespace m6 {

class SymbolTable;

struct Evaluator {
  Evaluator(std::shared_ptr<SymbolTable> sym_tab = nullptr);

  Value_ptr operator()(std::monostate) const;
  Value_ptr operator()(const IdExpr& str) const;
  Value_ptr operator()(int x) const;
  Value_ptr operator()(const std::string& x) const;
  Value_ptr operator()(const InvokeExpr& x) const;
  Value_ptr operator()(const SubscriptExpr& x) const;
  Value_ptr operator()(const MemberExpr& x) const;
  Value_ptr operator()(const ParenExpr& x) const;
  Value_ptr operator()(const UnaryExpr& x) const;
  Value_ptr operator()(const BinaryExpr& x) const;
  Value_ptr operator()(const AssignExpr& x) const;

  std::shared_ptr<SymbolTable> sym_tab_;
};

}  // namespace m6
