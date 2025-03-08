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
#include "m6/token.hpp"
#include "machine/op.hpp"
#include "machine/value.hpp"
#include "utilities/string_utilities.hpp"

#include <vector>

template <typename... Ts>
std::vector<m6::Token> TokenArray(Ts&&... args) {
  std::vector<m6::Token> result;
  result.reserve(sizeof...(args));
  (result.emplace_back(std::forward<Ts>(args)), ...);
  return result;
}

struct GetPrefix {
  std::string operator()(const m6::BinaryExpr& x) const;
  std::string operator()(const m6::UnaryExpr& x) const;
  std::string operator()(const m6::AssignExpr& x) const;
  std::string operator()(const m6::ParenExpr& x) const;
  std::string operator()(const m6::InvokeExpr& x) const;
  std::string operator()(const m6::SubscriptExpr& x) const;
  std::string operator()(const m6::MemberExpr& x) const;
  std::string operator()(std::monostate) const;
  std::string operator()(int x) const;
  std::string operator()(const std::string& str) const;
  std::string operator()(const m6::IdExpr& str) const;
};

inline bool Compare(Value_ptr lhs, Value_ptr rhs) noexcept {
  try {
    Value_ptr result = lhs->__Operator(Op::Equal, rhs);
    return std::any_cast<int>(result->Get()) != 0;
  } catch (...) {
    return false;
  }
}

#define EXPECT_VALUE_EQ(val, expected)                                      \
  EXPECT_TRUE(Compare(val, make_value(expected)))                           \
      << "Expected equality between: " << val->Str() << " and " << expected \
      << '\n'
