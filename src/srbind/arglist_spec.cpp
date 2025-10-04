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

#include "srbind/arglist_spec.hpp"

#include "utilities/overload.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/iobject.hpp"

namespace srbind {

static inline std::string str(serilang::TempValue val) {
  return std::visit(
      overload([](const serilang::Value& val) { return val.Str(); },
               [](const std::unique_ptr<serilang::IObject>& obj) {
                 return obj->Str();
               }),
      val);
}

std::string arglist_spec::GetDebugString() const {
  std::string result;
  if (has_vm)
    result += 'v';
  if (has_fib)
    result += 'f';

  result += '(';

  std::vector<std::string> params(nparam);
  for (size_t i = 0; i < nparam; ++i)
    params[i] = "arg_" + std::to_string(i);

  for (const auto& [name, idx] : param_index)
    params[idx] = name;
  for (const auto& [idx, fn] : defaults)
    params[idx] += "=" + str(fn());

  result += Join(",", params);

  result += ')';

  if (has_vararg)
    result += 'a';
  if (has_kwarg)
    result += 'k';
  return result;
}

}  // namespace srbind
