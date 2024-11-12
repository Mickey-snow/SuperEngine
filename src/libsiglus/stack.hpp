// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#ifndef SRC_LIBSIGLUS_STACK_HPP_
#define SRC_LIBSIGLUS_STACK_HPP_

#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"

#include <stack>
#include <exception>
#include <string>
#include <vector>

namespace libsiglus {

using ElementCode = std::vector<int>;

class StackUnderflow : public std::exception {
 public:
  const char* what() const noexcept override;
};

class Stack {
 public:
  Stack();
  ~Stack();

  void Clearint();
  void Clearstr();
  void Clear();

  Stack& Push(int value);
  Stack& Push(std::string value);
  Stack& PushMarker();
  Stack& Push(const ElementCode& elm);

  int Backint() const;
  int& Backint();
  int Popint();

  const std::string& Backstr() const;
  std::string& Backstr();
  std::string Popstr();

  Value Pop(Type type);

  ElementCode Backelm() const;
  ElementCode Popelm();

 private:
  std::vector<int> intstk_;
  std::vector<std::string> strstk_;
  std::stack<size_t> elm_point_;
};

}  // namespace libsiglus

#endif  // SRC_LIBSIGLUS_STACK_HPP_
