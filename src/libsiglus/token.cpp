// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "libsiglus/token.hpp"
#include "utilities/string_utilities.hpp"

#include <format>
#include <sstream>

namespace libsiglus::token {
std::string Command::ToDebugString() const {
  const std::string cmd_repr =
      std::format("cmd<{}>", Join(",", vals_to_string(elmcode.code)));

  return std::format("{} {} = {:<30} ;{}", ToString(Typeof(dst)), ToString(dst),
                     chain.ToDebugString(), cmd_repr);
}
std::string Name::ToDebugString() const {
  return "Name(" + ToString(str) + ')';
}
std::string Textout::ToDebugString() const {
  return std::format("Textout@{} ({})", kidoku, ToString(str));
}
std::string GetProperty::ToDebugString() const {
  auto repr = std::format("{} {} = {}", ToString(Typeof(dst)), ToString(dst),
                          chain.ToDebugString());
  return std::format("{:<30} ;<{}>", std::move(repr),
                     Join(",", vals_to_string(elmcode.code)));
}
std::string Goto::ToDebugString() const {
  return "goto .L" + std::to_string(label);
}
std::string GotoIf::ToDebugString() const {
  return std::format("{}({}) goto .L{}", cond ? "if" : "ifnot", ToString(src),
                     label);
}
std::string Gosub::ToDebugString() const {
  return std::format("gosub@.L{}({})", entry_id,
                     Join(",", vals_to_string(args)));
}
std::string Label::ToDebugString() const { return ".L" + std::to_string(id); }
std::string Operate1::ToDebugString() const {
  auto expr = std::format("{} {} = {} {}", ToString(Typeof(dst)), ToString(dst),
                          ToString(op), ToString(rhs));
  return val.has_value()
             ? std::format("{:<30} ;{}", std::move(expr), ToString(*val))
             : expr;
}
std::string Operate2::ToDebugString() const {
  auto expr =
      std::format("{} {} = {} {} {}", ToString(Typeof(dst)), ToString(dst),
                  ToString(lhs), ToString(op), ToString(rhs));
  return val.has_value()
             ? std::format("{:<30} ;{}", std::move(expr), ToString(*val))
             : expr;
}
std::string Assign::ToDebugString() const {
  std::string repr = dst.ToDebugString() + " = " + ToString(src);
  return std::format("{:<30} ;<{}>", std::move(repr),
                     Join(",", vals_to_string(dst_elmcode.code)));
}
std::string Duplicate::ToDebugString() const {
  return std::format("{} {} = {}", ToString(Typeof(dst)), ToString(dst),
                     ToString(src));
}
std::string Subroutine::ToDebugString() const {
  std::stringstream ss;
  ss << "====== SUBROUTINE " << name << " ======";
  for (size_t i = 0; i < args.size(); ++i) {
    ss << '\n' << "  arg_" << i << ": " << ToString(args[i]);
  }
  return ss.str();
}
std::string Return::ToDebugString() const {
  return std::format("ret ({})", Join(",", vals_to_string(ret_vals)));
}
std::string Eof::ToDebugString() const { return "<EOF>"; }
}  // namespace libsiglus::token
