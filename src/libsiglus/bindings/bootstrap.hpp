// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#pragma once

#include <string_view>

namespace libsiglus::binding {

[[maybe_unused]] constexpr std::string_view kLazyArrayClass = R"(
class {} {{
  fn __init__(self, klass){{
    self.klass = klass;
    self.storage = [];
  }}
  fn __getitem__(self, idx){{
    while(self.storage.len() <= idx) self.storage.append(nil);
    if(self.storage[idx] == nil) self.storage[idx] = self.klass();
    return self.storage[idx];
  }}
}}
)";

[[maybe_unused]] constexpr std::string_view kIndexedFactory = R"(
class {} {{
  fn __init__(self){{
    self.storage = [];
  }}
  fn __getitem__(self, idx){{
    while(self.storage.len() <= idx) self.storage.append(nil);
    if(self.storage[idx] == nil) self.storage[idx] = {}(idx);
    return self.storage[idx];
  }}
}}
)";

}  // namespace libsiglus::binding
