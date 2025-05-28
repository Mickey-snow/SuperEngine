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

namespace libsiglus::token {
// helper to convert a range of Value to string
static inline auto vals_to_string(std::vector<Value> const& vals) {
  return std::views::all(vals) |
         std::views::transform([](const Value& v) { return ToString(v); });
}

std::string Command::ToDebugString() const {
  const std::string cmd_repr = std::format(
      "cmd<{}:{}>", Join(",", view_to_string(elmcode)), overload_id);

  std::vector<std::string> args_repr;
  args_repr.reserve(arg.size() + named_arg.size());

  std::transform(arg.cbegin(), arg.cend(), std::back_inserter(args_repr),
                 [](const Value& value) { return ToString(value); });
  std::transform(named_arg.cbegin(), named_arg.cend(),
                 std::back_inserter(args_repr), [](const auto& it) {
                   return std::format("_{}={}", it.first, ToString(it.second));
                 });

  auto repr =
      std::format("{} {} = {}({})", ToString(return_type), ToString(dst),
                  elm->ToDebugString(), Join(",", args_repr));
  return std::format("{:<30} ;{}", std::move(repr), cmd_repr);
}
std::string Textout::ToDebugString() const {
  return std::format("Textout@{} ({})", kidoku, str);
}
std::string GetProperty::ToDebugString() const {
  auto repr = std::format("{} {} = {}", ToString(Typeof(dst)), ToString(dst),
                          elm->ToDebugString());
  return std::format("{:<30} ;<{}>", std::move(repr),
                     Join(",", view_to_string(elmcode)));
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
  std::string repr = dst->ToDebugString() + " = " + ToString(src);
  return std::format("{:<30} ;<{}>", std::move(repr),
                     Join(",", view_to_string(dst_elmcode)));
}
std::string Duplicate::ToDebugString() const {
  return std::format("{} {} = {}", ToString(Typeof(dst)), ToString(dst),
                     ToString(src));
}
std::string Subroutine::ToDebugString() const {
  return std::format("====== SUBROUTINE {} ======", name);
}
std::string Return::ToDebugString() const {
  return std::format("ret ({})", Join(",", vals_to_string(ret_vals)));
}
std::string Precall::ToDebugString() const {
  return std::format("precall {} {}", ToString(type), size);
}
}  // namespace libsiglus::token
