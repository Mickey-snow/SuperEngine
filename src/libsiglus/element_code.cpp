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

#include "libsiglus/element_code.hpp"

namespace libsiglus {

ElementCode::ElementCode() : code{}, force_bind(false), bind_ctx() {}

bool ElementCode::operator==(const ElementCode& rhs) const {
  return code == rhs.code;
}

void ElementCode::ForceBind(Invoke ctx) {
  force_bind = true;
  bind_ctx = std::move(ctx);
}

}  // namespace libsiglus
