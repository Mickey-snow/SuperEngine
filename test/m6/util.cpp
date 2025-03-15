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

#include "util.hpp"

#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"

std::string GetPrefix::operator()(const m6::BinaryExpr& x) const {
  return ToString(x.op) + ' ' + x.lhs->Apply(*this) + ' ' + x.rhs->Apply(*this);
}
std::string GetPrefix::operator()(const m6::AssignExpr& x) const {
  return "= " + x.lhs->Apply(*this) + ' ' + x.rhs->Apply(*this);
}
std::string GetPrefix::operator()(const m6::UnaryExpr& x) const {
  return ToString(x.op) + ' ' + x.sub->Apply(*this);
}
std::string GetPrefix::operator()(const m6::ParenExpr& x) const {
  return x.sub->Apply(*this);
}
std::string GetPrefix::operator()(const m6::InvokeExpr& x) const {
  return x.fn->Apply(*this) + '(' +
         Join(", ", x.args | std::views::transform([&](const auto& arg) {
                      return arg->Apply(*this);
                    })) +
         ')';
}
std::string GetPrefix::operator()(const m6::SubscriptExpr& x) const {
  return x.primary->Apply(*this) + '[' + x.index->Apply(*this) + ']';
}
std::string GetPrefix::operator()(const m6::MemberExpr& x) const {
  return x.primary->Apply(*this) + '.' + x.member->Apply(*this);
}
std::string GetPrefix::operator()(std::monostate) const { return "<null>"; }
std::string GetPrefix::operator()(int x) const { return std::to_string(x); }
std::string GetPrefix::operator()(const std::string& str) const {
  return '"' + str + '"';
}
std::string GetPrefix::operator()(const m6::IdExpr& id) const { return id.id; }
