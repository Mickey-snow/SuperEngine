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

namespace {
inline std::string get_cmd_repr(std::span<const Value> elmcode) {
  return std::format("cmd<{}>", Join(",", vals_to_string(elmcode)));
}
}  // namespace

std::string ElmAlias::ToDebugString() const {
  const std::string dump =
      std::format("alias.{} {} = {}", ToString(Typeof(dst)),
                  dst.ToDebugString(), chain.ToDebugString());
  const std::string comment = get_cmd_repr(elmcode.code);
  return std::format("{:<55} ;{}", dump, comment);
}
std::string Command::ToDebugString() const {
  const std::string dump =
      std::format("{} {} = {}", ToString(Typeof(dst)), dst.ToDebugString(),
                  chain.ToDebugString());
  const std::string comment = get_cmd_repr(elmcode.code);
  return std::format("{:<55} ;{}", dump, comment);
}
std::string Name::ToDebugString() const {
  return "Name(" + ToString(str) + ')';
}
std::string Textout::ToDebugString() const {
  return std::format("Textout@{} ({})", kidoku, ToString(str));
}
std::string GetProperty::ToDebugString() const {
  const std::string dump =
      std::format("{} {} = {}", ToString(Typeof(dst)), dst.ToDebugString(),
                  chain.ToDebugString());
  const std::string comment = get_cmd_repr(elmcode.code);
  return std::format("{:<55} ;{}", dump, comment);
}
std::string Goto::ToDebugString() const {
  return "goto .L" + std::to_string(label);
}
std::string GotoIf::ToDebugString() const {
  return std::format("{}({}) goto .L{}", cond ? "if" : "ifnot", ToString(src),
                     label);
}
std::string Gosub::ToDebugString() const {
  return std::format("{} {} = gosub@.L{}({})", ToString(Typeof(dst)),
                     dst.ToDebugString(), entry_id,
                     Join(",", vals_to_string(args)));
}
std::string Label::ToDebugString() const { return ".L" + std::to_string(id); }
std::string Operate1::ToDebugString() const {
  auto expr = std::format("{} {} = {} {}", ToString(Typeof(dst)),
                          dst.ToDebugString(), ToString(op), ToString(rhs));
  return val.has_value()
             ? std::format("{:<55} ;{}", std::move(expr), ToString(*val))
             : expr;
}
std::string Operate2::ToDebugString() const {
  auto expr = std::format("{} {} = {} {} {}", ToString(Typeof(dst)),
                          dst.ToDebugString(), ToString(lhs), ToString(op),
                          ToString(rhs));
  return val.has_value()
             ? std::format("{:<55} ;{}", std::move(expr), ToString(*val))
             : expr;
}
std::string Assign::ToDebugString() const {
  const std::string dump = dst.ToDebugString() + " = " + ToString(src);
  const std::string comment = Join(",", vals_to_string(dst_elmcode.code));
  return std::format("{:<55} ;<{}>", dump, comment);
}
std::string Duplicate::ToDebugString() const {
  return std::format("{} {} = {}", ToString(Typeof(dst)), dst.ToDebugString(),
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
