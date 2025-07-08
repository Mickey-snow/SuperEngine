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

#include "libsiglus/function.hpp"

#include "utilities/overload.hpp"
#include "utilities/string_utilities.hpp"

#include <algorithm>
#include <format>

namespace libsiglus::elm {

std::vector<std::string> ArgumentList::ToStringVec() const {
  std::vector<std::string> repr;
  repr.reserve(args.size());
  std::transform(args.cbegin(), args.cend(), std::back_inserter(repr),
                 [](const auto& x) {
                   return std::visit(
                       [](const auto& x) {
                         using T = std::decay_t<decltype(x)>;
                         if constexpr (std::same_as<T, Type>)
                           return ToString(x);
                         else
                           return '[' + x.ToDebugString() + ']';
                       },
                       x);
                 });
  return repr;
}
std::string ArgumentList::ToDebugString() const {
  return Join(",", ToStringVec());
}

std::string Signature::ToDebugString() const {
  std::vector<std::string> args_repr = arglist.ToStringVec();
  for (size_t i = arglist.size() - argtags.size(); i < arglist.size(); ++i) {
    args_repr[i] = std::format(
        "_{}={}", argtags[i - (arglist.size() - argtags.size())], args_repr[i]);
  }

  return std::format("[{}]({}) -> {}", overload_id,
                     Join(",", std::move(args_repr)), ToString(rettype));
}

Invoke::Invoke(int ol, std::vector<Value> arglist, Type rettype)
    : overload_id(ol), arg(std::move(arglist)), return_type(rettype) {}

std::string Invoke::ToDebugString(bool show_overload, bool show_rettype) const {
  std::vector<std::string> args_repr;
  args_repr.reserve(arg.size() + named_arg.size());

  std::transform(arg.cbegin(), arg.cend(), std::back_inserter(args_repr),
                 [](const Value& value) { return ToString(value); });
  std::transform(named_arg.cbegin(), named_arg.cend(),
                 std::back_inserter(args_repr), [](const auto& it) {
                   return std::format("_{}={}", it.first, ToString(it.second));
                 });

  std::string result = '(' + Join(",", args_repr) + ')';
  if (show_overload)
    result = '[' + std::to_string(overload_id) + ']';
  if (show_rettype)
    result += "->" + ToString(return_type);
  return result;
}

bool Invoke::Empty() const { return arg.empty() && named_arg.empty(); }

std::string Function::Arg::ToDebugString() const {
  return std::visit(
      ::overload([](const va_arg& x) { return ToString(x.type) + "..."; },
                 [](const kw_arg& x) {
                   return '_' + std::to_string(x.kw) + ':' + ToString(x.type);
                 },
                 [](const Type& x) { return ToString(x); }),
      arg);
}
std::string Function::ToDebugString() const {
  return std::format(
      "{}[{}]({})->{}", name,
      overload.has_value() ? std::to_string(*overload) : std::string(),
      Join(",",
           std::views::all(arg_t) | std::views::transform([](auto const& t) {
             return t.ToDebugString();
           })),
      ToString(return_t));
}
std::string Callable::ToDebugString() const {
  return ".<callable " +
         Join("  ", std::views::all(overloads) |
                        std::views::transform([](const auto& it) {
                          return it.ToDebugString();
                        })) +
         '>';
}

}  // namespace libsiglus::elm
