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

#include "m6/exception.hpp"
#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"

namespace m6 {

UndefinedOperator::UndefinedOperator(Op op, std::vector<std::string> operands)
    : std::logic_error("no match for 'operator " + ToString(op) +
                       "' (operand type '" + Join(",", operands)) {}

ValueError::ValueError(std::string msg) : std::runtime_error(std::move(msg)) {}

TypeError::TypeError(std::string msg) : std::runtime_error(std::move(msg)) {}

SyntaxError::SyntaxError(std::string msg) : std::logic_error(std::move(msg)) {}

NameError::NameError(const std::string& name)
    : std::runtime_error("name '" + name + "' is not defined.") {}

}  // namespace m6
