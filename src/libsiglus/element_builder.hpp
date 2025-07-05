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

#pragma once

#include "libsiglus/element.hpp"
#include "libsiglus/element_code.hpp"
#include "libsiglus/types.hpp"

#include <optional>
#include <span>

namespace libsiglus::elm {

AccessChain MakeChain(ElementCode& elm);
AccessChain MakeChain(elm::Root root,
                      ElementCode& elm,
                      std::span<const Value> elmcode);
AccessChain MakeChain(Type root_type,
                      Root::var_t root_node,
                      ElementCode& elm,
                      size_t subidx = 0);

}  // namespace libsiglus::elm
