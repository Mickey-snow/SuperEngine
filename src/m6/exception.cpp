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
#include "m6/token.hpp"
#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"

#include <sstream>

namespace m6 {

// -----------------------------------------------------------------------
// class SourceLocation
// -----------------------------------------------------------------------
SourceLocation::SourceLocation(const Token& tok)
    : begin_offset(tok.offset), end_offset(begin_offset + 1) {}
SourceLocation::SourceLocation(const Token& begin, const Token& end)
    : begin_offset(begin.offset), end_offset(end.offset) {}

// -----------------------------------------------------------------------
// class RuntimeError
// -----------------------------------------------------------------------
RuntimeError::RuntimeError(std::string msg) : msg_(std::move(msg)) {}
char const* RuntimeError::what() const noexcept { return msg_.c_str(); }

// -----------------------------------------------------------------------
// class CompileError
// -----------------------------------------------------------------------
CompileError::CompileError(std::string msg, std::optional<SourceLocation> loc)
    : msg_(std::move(msg)), loc_(loc) {}
char const* CompileError::what() const noexcept { return msg_.c_str(); }
std::optional<SourceLocation> CompileError::where() const noexcept {
  return loc_;
}
std::string CompileError::FormatWith(std::string_view src) const {
  std::ostringstream oss;

  // Print error message
  oss << "error: " << this->what() << '\n';
  auto loc = this->where();

  if (loc.has_value()) {
    // Print source code
    oss << src << '\n';
    // Print a caret under the error position.
    oss << std::string(loc->begin_offset, ' ')
        << std::string(loc->end_offset - loc->begin_offset, '^');
  }

  return oss.str();
}

// -----------------------------------------------------------------------

UndefinedOperator::UndefinedOperator(Op op, std::vector<std::string> operands)
    : RuntimeError("no match for 'operator " + ToString(op) +
                   "' (operand type '" + Join(",", operands)) {}

ValueError::ValueError(std::string msg) : RuntimeError(std::move(msg)) {}

TypeError::TypeError(std::string msg) : RuntimeError(std::move(msg)) {}

SyntaxError::SyntaxError(std::string msg, std::optional<SourceLocation> loc)
    : CompileError(std::move(msg), loc) {}

NameError::NameError(const std::string& name, std::optional<SourceLocation> loc)
    : CompileError("name '" + name + "' is not defined.", loc) {}

}  // namespace m6
