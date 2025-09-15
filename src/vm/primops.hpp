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

// Centralized primitive operator tables and helpers.

#include "machine/op.hpp"
#include "vm/value.hpp"

#include <optional>

namespace serilang::primops {

// Evaluate primitive binary op. Returns std::nullopt if not handled.
std::optional<Value> EvaluateBinary(Op op, const Value& lhs, const Value& rhs);

// Evaluate primitive unary op. Returns std::nullopt if not handled.
std::optional<Value> EvaluateUnary(Op op, const Value& v);

}  // namespace serilang::primops
