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

#include "m6/expr_ast.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace m6 {

class UndefinedOperator : public std::logic_error {
 public:
  explicit UndefinedOperator(Op op, std::vector<std::string> operands);
  using std::logic_error::what;
};

class ValueError : public std::runtime_error {
 public:
  explicit ValueError(std::string msg);
  using std::runtime_error::what;
};

class TypeError : public std::runtime_error {
 public:
  explicit TypeError(std::string msg);
  using std::runtime_error::what;
};

class SyntaxError : public std::logic_error {
 public:
  explicit SyntaxError(std::string msg);
  using std::logic_error::what;
};

class NameError : public std::runtime_error {
 public:
  explicit NameError(const std::string& name);
  using std::runtime_error::what;
};

}  // namespace m6
