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

// -----------------------------------------------------------------------
// class RuntimeError
// -----------------------------------------------------------------------
RuntimeError::RuntimeError(std::string msg) : msg_(std::move(msg)) {}
char const* RuntimeError::what() const noexcept { return msg_.c_str(); }

// -----------------------------------------------------------------------
// class CompileError
// -----------------------------------------------------------------------
CompileError::CompileError(std::string msg, Token* tok)
    : msg_(std::move(msg)), tok_(tok) {}
char const* CompileError::what() const noexcept { return msg_.c_str(); }
Token const* CompileError::where() const noexcept { return tok_; }

// -----------------------------------------------------------------------

UndefinedOperator::UndefinedOperator(Op op, std::vector<std::string> operands)
    : RuntimeError("no match for 'operator " + ToString(op) +
                   "' (operand type '" + Join(",", operands)) {}

ValueError::ValueError(std::string msg) : RuntimeError(std::move(msg)) {}

TypeError::TypeError(std::string msg) : RuntimeError(std::move(msg)) {}

SyntaxError::SyntaxError(std::string msg, Token* tok)
    : CompileError(std::move(msg), tok) {}

NameError::NameError(const std::string& name, Token* tok)
    : CompileError("name '" + name + "' is not defined.", tok) {}

}  // namespace m6
