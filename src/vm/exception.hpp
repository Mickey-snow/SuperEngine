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

#include "machine/op.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace serilang {

class RuntimeError : public std::exception {
  std::string msg_;

 public:
  explicit RuntimeError(std::string msg);
  virtual const char* what() const noexcept { return msg_.data(); }
  std::string const& message() const { return msg_; }
};

class ValueError : public RuntimeError {
 public:
  explicit ValueError(std::string msg);
  using RuntimeError::what;
};

class UndefinedOperator : public RuntimeError {
 public:
  explicit UndefinedOperator(Op op, std::vector<std::string> operands);
  using RuntimeError::what;
};

}  // namespace serilang
